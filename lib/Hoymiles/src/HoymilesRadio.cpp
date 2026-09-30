// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2026 Thomas Basler and others
 */
#include "HoymilesRadio.h"
#include "crc.h"
#include "Hoymiles.h"
#include <esp_log.h>

#undef TAG
static const char* TAG = "hoymiles";

serial_u HoymilesRadio::DtuSerial() const
{
    return _dtuSerial;
}

void HoymilesRadio::setDtuSerial(const uint64_t serial)
{
    _dtuSerial.u64 = serial;
}

serial_u HoymilesRadio::convertSerialToRadioId(const serial_u serial)
{
    serial_u radioId;
    radioId.u64 = 0;
    radioId.b[4] = serial.b[0];
    radioId.b[3] = serial.b[1];
    radioId.b[2] = serial.b[2];
    radioId.b[1] = serial.b[3];
    radioId.b[0] = 0x01;
    return radioId;
}

bool HoymilesRadio::checkFragmentCrc(const fragment_t& fragment) const
{
    if (fragment.len == 0 || fragment.len > MAX_RF_PAYLOAD_SIZE) {
        return false;
    }

    const uint8_t crc = crc8(fragment.fragment, fragment.len - 1);
    return (crc == fragment.fragment[fragment.len - 1]);
}

void HoymilesRadio::sendRetransmitPacket(const uint8_t fragment_id)
{
    auto front = _commandQueue.tryFront();
    if (!front.has_value()) {
        // The command was removed from the queue while its transaction was
        // in flight (e.g. its inverter got deleted). Abort the transaction.
        _busyFlag = false;
        return;
    }

    CommandAbstract* requestCmd = front.value().get()->getRequestFrameCommand(fragment_id);

    if (requestCmd != nullptr) {
        sendEsbPacket(*requestCmd);
    }
}

void HoymilesRadio::sendLastPacketAgain()
{
    auto front = _commandQueue.tryFront();
    if (!front.has_value()) {
        // The command was removed from the queue while its transaction was
        // in flight (e.g. its inverter got deleted). Abort the transaction.
        _busyFlag = false;
        return;
    }
    sendEsbPacket(*front.value().get());
}

void HoymilesRadio::handleReceivedPackage()
{
    // Early exit: if the inverter already received and stored all expected
    // fragments of the current transaction (the last fragment of a response
    // carries the 0x80 flag and every fragment up to it was seen), waiting
    // for the rx timeout would be pure latency. Finalize the transaction
    // immediately once the radio's rx buffer is fully drained so that no
    // fragment of this response can be attributed to the next transaction.
    // The timeout below remains as fallback for incomplete responses.
    bool earlyExit = false;
    if (_busyFlag && !_rxTimeout.occured() && isRxBufferEmpty()) {
        auto front = _commandQueue.tryFront();
        if (front.has_value()) {
            std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(front.value().get()->getTargetAddress());
            if (nullptr != inv && inv->isTransactionComplete()) {
                earlyExit = true;
                ESP_LOGI(TAG, "RX Early Exit");
            }
        }
    }

    if (_busyFlag && (earlyExit || _rxTimeout.occured())) {
        if (!earlyExit) {
            ESP_LOGI(TAG, "RX Period End");
        }
        auto front = _commandQueue.tryFront();
        if (!front.has_value()) {
            // The command was removed from the queue while its transaction
            // was in flight (e.g. its inverter got deleted). Abort the
            // transaction instead of dereferencing an empty queue.
            _busyFlag = false;
            return;
        }
        std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(front.value().get()->getTargetAddress());

        if (nullptr != inv) {
            CommandAbstract* cmd = front.value().get();
            uint8_t verifyResult = inv->verifyAllFragments(*cmd);
            if (verifyResult == FRAGMENT_ALL_MISSING_RESEND) {
                ESP_LOGW(TAG, "Nothing received, resend whole request");
                sendLastPacketAgain();

            } else if (verifyResult == FRAGMENT_ALL_MISSING_TIMEOUT) {
                ESP_LOGW(TAG, "Nothing received, resend count exeeded");
                // Statistics: Count RX Fail No Answer
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailNoAnswer++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_RETRANSMIT_TIMEOUT) {
                ESP_LOGW(TAG, "Retransmit timeout");
                // Statistics: Count RX Fail Partial Answer
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailPartialAnswer++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_HANDLE_ERROR) {
                ESP_LOGW(TAG, "Packet handling error");
                // Statistics: Count RX Fail Corrupt Data
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailCorruptData++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult > 0) {
                // Perform Retransmit
                ESP_LOGI(TAG, "Request retransmit: %" PRIu8 "", verifyResult);
                // Statistics: Count TX Re-Request Fragment
                inv->RadioStats.TxReRequestFragment++;

                sendRetransmitPacket(verifyResult);

            } else {
                // Successful received all packages
                ESP_LOGI(TAG, "Success");
                // Statistics: Count RX Success
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxSuccess++;
                }

                _commandQueue.pop();
                _busyFlag = false;
            }
        } else {
            // If inverter was not found, assume the command is invalid
            ESP_LOGW(TAG, "RX: Invalid inverter found");
            // Statistics: Count RX Fail Unknown Data
            _commandQueue.pop();
            _busyFlag = false;
        }
    } else if (!_busyFlag) {
        // Currently in idle mode --> send packet if one is in the queue
        if (!isQueueEmpty()) {
            auto front = _commandQueue.tryFront();
            if (!front.has_value()) {
                return;
            }
            CommandAbstract* cmd = front.value().get();

            auto inv = Hoymiles.getInverterBySerial(cmd->getTargetAddress());
            if (nullptr != inv) {
                inv->clearRxFragmentBuffer();
                // Statistics: TX Requests
                inv->RadioStats.TxRequestData++;

                sendEsbPacket(*cmd);
            } else {
                ESP_LOGE(TAG, "TX: Invalid inverter found");
                _commandQueue.pop();
            }
        }
    }
}

bool HoymilesRadio::isInitialized() const
{
    return _isInitialized;
}

void HoymilesRadio::removeCommands(InverterAbstract* inv)
{
    _commandQueue.removeAllEntriesForInverter(inv);
}

uint8_t HoymilesRadio::countSimilarCommands(std::shared_ptr<CommandAbstract> cmd)
{
    return _commandQueue.countSimilarCommands(cmd);
}

bool HoymilesRadio::isIdle() const
{
    return !_busyFlag;
}

bool HoymilesRadio::isQueueEmpty() const
{
    return _commandQueue.size() == 0;
}

uint32_t HoymilesRadio::getQueueSize() const
{
    return _commandQueue.size();
}

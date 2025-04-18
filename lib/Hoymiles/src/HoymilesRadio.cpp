// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HoymilesRadio.h"
#include "Hoymiles.h"
#include "crc.h"

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
    const uint8_t crc = crc8(fragment.fragment, fragment.len - 1);
    return (crc == fragment.fragment[fragment.len - 1]);
}

void HoymilesRadio::sendRetransmitPacket(const uint8_t fragment_id)
{
    CommandAbstract* cmd = _commandQueue.front().get();

    CommandAbstract* requestCmd = cmd->getRequestFrameCommand(fragment_id);

    if (requestCmd != nullptr) {
        sendEsbPacket(*requestCmd);
    }
}

void HoymilesRadio::sendLastPacketAgain()
{
    CommandAbstract* cmd = _commandQueue.front().get();
    sendEsbPacket(*cmd);
}

void HoymilesRadio::handleReceivedPackage()
{
    if (_busyFlag && _rxTimeout.occured()) {
        Hoymiles.getMessageOutput()->printf("RX Period End\n");
        std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(_commandQueue.front().get()->getTargetAddress());

        if (nullptr != inv) {
            CommandAbstract* cmd = _commandQueue.front().get();
            uint8_t verifyResult = inv->verifyAllFragments(*cmd);
            if (verifyResult == FRAGMENT_ALL_MISSING_RESEND) {
                Hoymiles.getMessageOutput()->printf("Nothing received, resend whole request\n");
                sendLastPacketAgain();

            } else if (verifyResult == FRAGMENT_ALL_MISSING_TIMEOUT) {
                Hoymiles.getMessageOutput()->printf("Nothing received, resend count exeeded\n");
                // Statistics: Count RX Fail No Answer
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailNoAnswer++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_RETRANSMIT_TIMEOUT) {
                Hoymiles.getMessageOutput()->printf("Retransmit timeout\n");
                // Statistics: Count RX Fail Partial Answer
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailPartialAnswer++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_HANDLE_ERROR) {
                Hoymiles.getMessageOutput()->printf("Packet handling error\n");
                // Statistics: Count RX Fail Corrupt Data
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxFailCorruptData++;
                }

                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult > 0) {
                // Perform Retransmit
                Hoymiles.getMessageOutput()->printf("Request retransmit: %" PRIu8 "\n", verifyResult);
                // Statistics: Count TX Re-Request Fragment
                inv->RadioStats.TxReRequestFragment++;

                sendRetransmitPacket(verifyResult);

            } else {
                // Successful received all packages
                Hoymiles.getMessageOutput()->printf("Success\n");
                // Statistics: Count RX Success
                if (inv->RadioStats.TxRequestData > 0) {
                    inv->RadioStats.RxSuccess++;
                }

                _commandQueue.pop();
                _busyFlag = false;
            }
        } else {
            // If inverter was not found, assume the command is invalid
            Hoymiles.getMessageOutput()->printf("RX: Invalid inverter found\n");
            // Statistics: Count RX Fail Unknown Data
            _commandQueue.pop();
            _busyFlag = false;
        }
    } else if (!_busyFlag) {
        // Currently in idle mode --> send packet if one is in the queue
        if (!isQueueEmpty()) {
            CommandAbstract* cmd = _commandQueue.front().get();

            auto inv = Hoymiles.getInverterBySerial(cmd->getTargetAddress());
            if (nullptr != inv) {
                inv->clearRxFragmentBuffer();
                // Statistics: TX Requests
                inv->RadioStats.TxRequestData++;

                sendEsbPacket(*cmd);
            } else {
                Hoymiles.getMessageOutput()->printf("TX: Invalid inverter found\n");
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

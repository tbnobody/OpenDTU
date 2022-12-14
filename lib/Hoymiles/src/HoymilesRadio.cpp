// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "HoymilesRadio.h"
#include "Hoymiles.h"
#include "commands/RequestFrameCommand.h"
#include "crc.h"
#include <Every.h>
#include <FunctionalInterrupt.h>

void HoymilesRadio::init(SPIClass* initialisedSpiBus, uint8_t pinCE, uint8_t pinIRQ)
{
    _dtuSerial.u64 = 0;

    _spiPtr.reset(initialisedSpiBus);
    _radio.reset(new RF24(pinCE, initialisedSpiBus->pinSS()));

    _radio->begin(_spiPtr.get());

    _radio->setDataRate(RF24_250KBPS);
    _radio->enableDynamicPayloads();
    _radio->setCRCLength(RF24_CRC_16);
    _radio->setAddressWidth(5);
    _radio->setRetries(0, 0);
    _radio->maskIRQ(true, true, false); // enable only receiving interrupts
    if (_radio->isChipConnected()) {
        Serial.println(F("Connection successfull"));
    } else {
        Serial.println(F("Connection error!!"));
    }

    attachInterrupt(digitalPinToInterrupt(pinIRQ), std::bind(&HoymilesRadio::handleIntr, this), FALLING);

    openReadingPipe();
    _radio->startListening();
}

void HoymilesRadio::loop()
{
    EVERY_N_MILLIS(4)
    {
        switchRxCh();
    }

    if (_packetReceived) {
        Serial.println(F("Interrupt received"));
        while (_radio->available()) {
            if (!(_rxBuffer.size() > FRAGMENT_BUFFER_SIZE)) {
                fragment_t f;
                memset(f.fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
                f.len = _radio->getDynamicPayloadSize();
                f.channel = _radio->getChannel();
                if (f.len > MAX_RF_PAYLOAD_SIZE)
                    f.len = MAX_RF_PAYLOAD_SIZE;
                _radio->read(f.fragment, f.len);
                _rxBuffer.push(f);
            } else {
                Serial.println(F("Buffer full"));
                _radio->flush_rx();
            }
        }
        _packetReceived = false;

    } else {
        // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.back();
            if (checkFragmentCrc(&f)) {
                std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(&f);

                if (nullptr != inv) {
                    // Save packet in inverter rx buffer
                    char buf[30];
                    snprintf(buf, sizeof(buf), "RX Channel: %d --> ", f.channel);
                    dumpBuf(buf, f.fragment, f.len);
                    inv->addRxFragment(f.fragment, f.len);
                } else {
                    Serial.println(F("Inverter Not found!"));
                }

            } else {
                Serial.println(F("Frame kaputt"));
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    if (_busyFlag && _rxTimeout.occured()) {
        Serial.println(F("RX Period End"));
        std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(_commandQueue.front().get()->getTargetAddress());

        if (nullptr != inv) {
            CommandAbstract* cmd = _commandQueue.front().get();
            uint8_t verifyResult = inv->verifyAllFragments(cmd);
            if (verifyResult == FRAGMENT_ALL_MISSING_RESEND) {
                Serial.println(F("Nothing received, resend whole request"));
                sendLastPacketAgain();

            } else if (verifyResult == FRAGMENT_ALL_MISSING_TIMEOUT) {
                Serial.println(F("Nothing received, resend count exeeded"));
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_RETRANSMIT_TIMEOUT) {
                Serial.println(F("Retransmit timeout"));
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_HANDLE_ERROR) {
                Serial.println(F("Packet handling error"));
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult > 0) {
                // Perform Retransmit
                Serial.print(F("Request retransmit: "));
                Serial.println(verifyResult);
                sendRetransmitPacket(verifyResult);

            } else {
                // Successfull received all packages
                Serial.println(F("Success"));
                _commandQueue.pop();
                _busyFlag = false;
            }
        } else {
            // If inverter was not found, assume the command is invalid
            Serial.println(F("RX: Invalid inverter found"));
            _commandQueue.pop();
            _busyFlag = false;
        }
    } else if (!_busyFlag) {
        // Currently in idle mode --> send packet if one is in the queue
        if (!_commandQueue.empty()) {
            CommandAbstract* cmd = _commandQueue.front().get();

            auto inv = Hoymiles.getInverterBySerial(cmd->getTargetAddress());
            if (nullptr != inv) {
                inv->clearRxFragmentBuffer();
                sendEsbPacket(cmd);
            } else {
                Serial.println(F("TX: Invalid inverter found"));
                _commandQueue.pop();
            }
        }
    }
}

void HoymilesRadio::setPALevel(rf24_pa_dbm_e paLevel)
{
    _radio->setPALevel(paLevel);
}

serial_u HoymilesRadio::DtuSerial()
{
    return _dtuSerial;
}

void HoymilesRadio::setDtuSerial(uint64_t serial)
{
    _dtuSerial.u64 = serial;
    openReadingPipe();
}

bool HoymilesRadio::isIdle()
{
    return !_busyFlag;
}

bool HoymilesRadio::isConnected()
{
    return _radio->isChipConnected();
}

bool HoymilesRadio::isPVariant()
{
    return _radio->isPVariant();
}

void HoymilesRadio::openReadingPipe()
{
    serial_u s;
    s = convertSerialToRadioId(_dtuSerial);
    _radio->openReadingPipe(1, s.u64);
}

void HoymilesRadio::openWritingPipe(serial_u serial)
{
    serial_u s;
    s = convertSerialToRadioId(serial);
    _radio->openWritingPipe(s.u64);
}

void ARDUINO_ISR_ATTR HoymilesRadio::handleIntr()
{
    _packetReceived = true;
}

uint8_t HoymilesRadio::getRxNxtChannel()
{
    if (++_rxChIdx >= sizeof(_rxChLst))
        _rxChIdx = 0;
    return _rxChLst[_rxChIdx];
}

uint8_t HoymilesRadio::getTxNxtChannel()
{
    if (++_txChIdx >= sizeof(_txChLst))
        _txChIdx = 0;
    return _txChLst[_txChIdx];
}

void HoymilesRadio::switchRxCh()
{
    _radio->stopListening();
    _radio->setChannel(getRxNxtChannel());
    _radio->startListening();
}

serial_u HoymilesRadio::convertSerialToRadioId(serial_u serial)
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

bool HoymilesRadio::checkFragmentCrc(fragment_t* fragment)
{
    uint8_t crc = crc8(fragment->fragment, fragment->len - 1);
    return (crc == fragment->fragment[fragment->len - 1]);
}

void HoymilesRadio::sendEsbPacket(CommandAbstract* cmd)
{
    cmd->incrementSendCount();

    cmd->setRouterAddress(DtuSerial().u64);

    _radio->stopListening();
    _radio->setChannel(getTxNxtChannel());

    serial_u s;
    s.u64 = cmd->getTargetAddress();
    openWritingPipe(s);
    _radio->setRetries(3, 15);

    Serial.print(F("TX "));
    Serial.print(cmd->getCommandName());
    Serial.print(F(" Channel: "));
    Serial.print(_radio->getChannel());
    Serial.print(F(" --> "));
    cmd->dumpDataPayload(Serial);
    _radio->write(cmd->getDataPayload(), cmd->getDataSize());

    _radio->setRetries(0, 0);
    openReadingPipe();
    _radio->setChannel(getRxNxtChannel());
    _radio->startListening();
    _busyFlag = true;
    _rxTimeout.set(cmd->getTimeout());
}

void HoymilesRadio::sendRetransmitPacket(uint8_t fragment_id)
{
    CommandAbstract* cmd = _commandQueue.front().get();

    CommandAbstract* requestCmd = cmd->getRequestFrameCommand(fragment_id);

    if (requestCmd != nullptr) {
        sendEsbPacket(requestCmd);
    }
}

void HoymilesRadio::sendLastPacketAgain()
{
    CommandAbstract* cmd = _commandQueue.front().get();
    sendEsbPacket(cmd);
}

void HoymilesRadio::dumpBuf(const char* info, uint8_t buf[], uint8_t len)
{

    if (NULL != info)
        Serial.print(String(info));

    for (uint8_t i = 0; i < len; i++) {
        Serial.printf("%02X ", buf[i]);
    }
    Serial.println(F(""));
}
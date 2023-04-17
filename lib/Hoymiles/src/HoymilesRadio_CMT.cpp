// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "crc.h"
#include <FunctionalInterrupt.h>

#define HOY_BOOT_FREQ 868000000 // Hoymiles boot/init frequency after power up inverter or connection lost for 15 min
#define HOY_BASE_FREQ 860000000
// offset from initalized CMT base frequency to Hoy base frequency in channels
#define CMT_BASE_CH_OFFSET860 ((CMT_BASE_FREQ - HOY_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET)

// frequency can not be lower than actual initailized base freq
#define MIN_FREQ_KHZ ((HOY_BASE_FREQ + (CMT_BASE_CH_OFFSET860 >= 1 ? CMT_BASE_CH_OFFSET860 : 1) * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000)

// =923500, 0xFF does not work
#define MAX_FREQ_KHZ ((HOY_BASE_FREQ + 0xFE * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000)

float HoymilesRadio_CMT::getFrequencyFromChannel(const uint8_t channel)
{
    return (CMT_BASE_FREQ + (CMT_BASE_CH_OFFSET860 + channel) * FH_OFFSET * CMT2300A_ONE_STEP_SIZE) / 1000000.0;
}

uint8_t HoymilesRadio_CMT::getChannelFromFrequency(const uint32_t freq_kHz)
{
    if ((freq_kHz % 250) != 0) {
        Hoymiles.getMessageOutput()->printf("%.3f MHz is not divisible by 250 kHz!\r\n", freq_kHz / 1000.0);
        return 0xFF; // ERROR
    }
    if (freq_kHz < MIN_FREQ_KHZ || freq_kHz > MAX_FREQ_KHZ) {
        Hoymiles.getMessageOutput()->printf("%.2f MHz is out of Hoymiles/CMT range! (%.2f MHz - %.2f MHz)\r\n",
            freq_kHz / 1000.0, MIN_FREQ_KHZ / 1000.0, MAX_FREQ_KHZ / 1000.0);
        return 0xFF; // ERROR
    }
    if (freq_kHz < 863000 || freq_kHz > 870000) {
        Hoymiles.getMessageOutput()->printf("!!! caution: %.2f MHz is out of EU legal range! (863 - 870 MHz)\r\n",
            freq_kHz / 1000.0);
    }
    return (freq_kHz * 1000 - CMT_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET - CMT_BASE_CH_OFFSET860; // frequency to channel
}

bool HoymilesRadio_CMT::cmtSwitchDtuFreq(const uint32_t to_freq_kHz)
{
    const uint8_t toChannel = getChannelFromFrequency(to_freq_kHz);
    if (toChannel == 0xFF) {
        return false;
    }

    _radio->setChannel(toChannel);

    return true;
}

void HoymilesRadio_CMT::init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, int8_t pin_gpio2, int8_t pin_gpio3)
{
    _dtuSerial.u64 = 0;

    _radio.reset(new CMT2300A(pin_sdio, pin_clk, pin_cs, pin_fcs));

    _radio->begin();

    cmtSwitchDtuFreq(_inverterTargetFrequency); // start dtu at work freqency, for fast Rx if inverter is already on and frequency switched

    _isConfigured = true;
    if (!_radio->isChipConnected()) {
        Hoymiles.getMessageOutput()->println("CMT: Connection error!!");
        return;
    }
    Hoymiles.getMessageOutput()->println("CMT: Connection successful");

    if (pin_gpio2 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_gpio2), std::bind(&HoymilesRadio_CMT::handleInt1, this), RISING);
        _gpio2_configured = true;
    }

    if (pin_gpio3 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_gpio3), std::bind(&HoymilesRadio_CMT::handleInt2, this), RISING);
        _gpio3_configured = true;
    }

    _isInitialized = true;
}

void HoymilesRadio_CMT::loop()
{
    if (!_isInitialized) {
        return;
    }

    if (!_gpio3_configured) {
        if (_radio->rxFifoAvailable()) { // read INT2, PKT_OK flag
            _packetReceived = true;
        }
    }

    if (_packetReceived) {
        Hoymiles.getMessageOutput()->println("Interrupt received");
        while (_radio->available()) {
            if (!(_rxBuffer.size() > FRAGMENT_BUFFER_SIZE)) {
                fragment_t f;
                memset(f.fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
                f.len = _radio->getDynamicPayloadSize();
                f.channel = _radio->getChannel();
                f.rssi = _radio->getRssiDBm();
                if (f.len > MAX_RF_PAYLOAD_SIZE) {
                    f.len = MAX_RF_PAYLOAD_SIZE;
                }
                _radio->read(f.fragment, f.len);
                _rxBuffer.push(f);
            } else {
                Hoymiles.getMessageOutput()->println("CMT: Buffer full");
                _radio->flush_rx();
            }
        }
        _radio->flush_rx();
        _packetReceived = false;

    } else {
        // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.back();
            if (checkFragmentCrc(&f)) {
                std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(&f);

                if (nullptr != inv) {
                    // Save packet in inverter rx buffer
                    Hoymiles.getMessageOutput()->printf("RX %.2f MHz --> ", getFrequencyFromChannel(f.channel));
                    dumpBuf(f.fragment, f.len, false);
                    Hoymiles.getMessageOutput()->printf("| %d dBm\r\n", f.rssi);

                    inv->addRxFragment(f.fragment, f.len);
                } else {
                    Hoymiles.getMessageOutput()->println("Inverter Not found!");
                }

            } else {
                Hoymiles.getMessageOutput()->println("Frame kaputt"); // ;-)
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    if (_busyFlag && _rxTimeout.occured()) {
        Hoymiles.getMessageOutput()->println("RX Period End");
        std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(_commandQueue.front().get()->getTargetAddress());

        if (nullptr != inv) {
            CommandAbstract* cmd = _commandQueue.front().get();
            uint8_t verifyResult = inv->verifyAllFragments(cmd);
            if (verifyResult == FRAGMENT_ALL_MISSING_RESEND) {
                Hoymiles.getMessageOutput()->println("Nothing received, resend whole request");
                sendLastPacketAgain();

            } else if (verifyResult == FRAGMENT_ALL_MISSING_TIMEOUT) {
                Hoymiles.getMessageOutput()->println("Nothing received, resend count exeeded");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_RETRANSMIT_TIMEOUT) {
                Hoymiles.getMessageOutput()->println("Retransmit timeout");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_HANDLE_ERROR) {
                Hoymiles.getMessageOutput()->println("Packet handling error");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult > 0) {
                // Perform Retransmit
                Hoymiles.getMessageOutput()->print("Request retransmit: ");
                Hoymiles.getMessageOutput()->println(verifyResult);
                sendRetransmitPacket(verifyResult);

            } else {
                // Successful received all packages
                Hoymiles.getMessageOutput()->println("Success");
                _commandQueue.pop();
                _busyFlag = false;
            }
        } else {
            // If inverter was not found, assume the command is invalid
            Hoymiles.getMessageOutput()->println("RX: Invalid inverter found");
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
                Hoymiles.getMessageOutput()->println("TX: Invalid inverter found");
                _commandQueue.pop();
            }
        }
    }
}

void HoymilesRadio_CMT::setPALevel(int8_t paLevel)
{
    if (!_isInitialized) {
        return;
    }

    if (_radio->setPALevel(paLevel)) {
        Hoymiles.getMessageOutput()->printf("CMT TX power set to %d dBm\r\n", paLevel);
    } else {
        Hoymiles.getMessageOutput()->printf("CMT TX power %d dBm is not defined! (min: -10 dBm, max: 20 dBm)\r\n", paLevel);
    }
}

void HoymilesRadio_CMT::setInverterTargetFrequency(uint32_t frequency)
{
    _inverterTargetFrequency = frequency;
    if (!_isInitialized) {
        return;
    }
    cmtSwitchDtuFreq(_inverterTargetFrequency);
}

uint32_t HoymilesRadio_CMT::getInverterTargetFrequency()
{
    return _inverterTargetFrequency;
}

bool HoymilesRadio_CMT::isConnected()
{
    if (!_isInitialized) {
        return false;
    }
    return _radio->isChipConnected();
}

uint32_t HoymilesRadio_CMT::getMinFrequency()
{
    return MIN_FREQ_KHZ;
}

uint32_t HoymilesRadio_CMT::getMaxFrequency()
{
    return MAX_FREQ_KHZ;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt1()
{
    _packetSent = true;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt2()
{
    _packetReceived = true;
}

void HoymilesRadio_CMT::sendEsbPacket(CommandAbstract* cmd)
{
    cmd->incrementSendCount();

    cmd->setRouterAddress(DtuSerial().u64);

    _radio->stopListening();

    if (cmd->getDataPayload()[0] == 0x56) { // @todo(tbnobody) Bad hack to identify ChannelChange Command
        cmtSwitchDtuFreq(HOY_BOOT_FREQ / 1000);
    }

    Hoymiles.getMessageOutput()->printf("TX %s %.2f MHz --> ",
        cmd->getCommandName().c_str(), getFrequencyFromChannel(_radio->getChannel()));
    cmd->dumpDataPayload(Hoymiles.getMessageOutput());

    if (!_radio->write(cmd->getDataPayload(), cmd->getDataSize())) {
        Hoymiles.getMessageOutput()->println("TX SPI Timeout");
    }
    cmtSwitchDtuFreq(_inverterTargetFrequency);
    _radio->startListening();
    _busyFlag = true;
    _rxTimeout.set(cmd->getTimeout());
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "crc.h"
#include <FunctionalInterrupt.h>

#define HOY_BOOT_FREQ 868000000 // Hoymiles boot/init frequency after power up inverter or connection lost for 15 min
#define HOY_BASE_FREQ 860000000
// offset from initalized CMT base frequency to Hoy base frequency in channels
#define CMT_BASE_CH_OFFSET ((CMT_BASE_FREQ - HOY_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET)

// frequency can not be lower than actual initailized base freq + 250000
#define MIN_FREQ ((HOY_BASE_FREQ + (CMT_BASE_CH_OFFSET > 1 ? CMT_BASE_CH_OFFSET : 1) * CMT2300A_ONE_STEP_SIZE * FH_OFFSET))

// =923500, 0xFF does not work
#define MAX_FREQ ((HOY_BASE_FREQ + 0xFE * CMT2300A_ONE_STEP_SIZE * FH_OFFSET))

uint32_t HoymilesRadio_CMT::getFrequencyFromChannel(const uint8_t channel)
{
    return (CMT_BASE_FREQ + (CMT_BASE_CH_OFFSET + channel) * FH_OFFSET * CMT2300A_ONE_STEP_SIZE);
}

uint8_t HoymilesRadio_CMT::getChannelFromFrequency(const uint32_t frequency)
{
    if ((frequency % 250000) != 0) {
        Hoymiles.getMessageOutput()->printf("%.3f MHz is not divisible by 250 kHz!\r\n", frequency / 1000000.0);
        return 0xFF; // ERROR
    }
    if (frequency < MIN_FREQ || frequency > MAX_FREQ) {
        Hoymiles.getMessageOutput()->printf("%.2f MHz is out of Hoymiles/CMT range! (%.2f MHz - %.2f MHz)\r\n",
            frequency / 1000000.0, MIN_FREQ / 1000000.0, MAX_FREQ / 1000000.0);
        return 0xFF; // ERROR
    }
    if (frequency < 863000000 || frequency > 870000000) {
        Hoymiles.getMessageOutput()->printf("!!! caution: %.2f MHz is out of EU legal range! (863 - 870 MHz)\r\n",
            frequency / 1000000.0);
    }
    return (frequency - CMT_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET - CMT_BASE_CH_OFFSET; // frequency to channel
}

bool HoymilesRadio_CMT::cmtSwitchDtuFreq(const uint32_t to_frequency)
{
    const uint8_t toChannel = getChannelFromFrequency(to_frequency);
    if (toChannel == 0xFF) {
        return false;
    }

    _radio->setChannel(toChannel);

    return true;
}

void HoymilesRadio_CMT::init(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const int8_t pin_gpio2, const int8_t pin_gpio3)
{
    _dtuSerial.u64 = 0;

    _radio.reset(new CMT2300A(pin_sdio, pin_clk, pin_cs, pin_fcs));

    _radio->begin();

    cmtSwitchDtuFreq(_inverterTargetFrequency); // start dtu at work freqency, for fast Rx if inverter is already on and frequency switched

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
                f.wasReceived = false;
                f.mainCmd = 0x00;
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
            if (checkFragmentCrc(f)) {

                const serial_u dtuId = convertSerialToRadioId(_dtuSerial);

                // The CMT RF module does not filter foreign packages by itself.
                // Has to be done manually here.
                if (memcmp(&f.fragment[5], &dtuId.b[1], 4) == 0) {

                    std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(f);

                    if (nullptr != inv) {
                        // Save packet in inverter rx buffer
                        Hoymiles.getMessageOutput()->printf("RX %.2f MHz --> ", getFrequencyFromChannel(f.channel) / 1000000.0);
                        dumpBuf(f.fragment, f.len, false);
                        Hoymiles.getMessageOutput()->printf("| %d dBm\r\n", f.rssi);

                        inv->addRxFragment(f.fragment, f.len);
                    } else {
                        Hoymiles.getMessageOutput()->println("Inverter Not found!");
                    }
                }

            } else {
                Hoymiles.getMessageOutput()->println("Frame kaputt"); // ;-)
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    handleReceivedPackage();
}

void HoymilesRadio_CMT::setPALevel(const int8_t paLevel)
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

void HoymilesRadio_CMT::setInverterTargetFrequency(const uint32_t frequency)
{
    _inverterTargetFrequency = frequency;
    if (!_isInitialized) {
        return;
    }
    cmtSwitchDtuFreq(_inverterTargetFrequency);
}

uint32_t HoymilesRadio_CMT::getInverterTargetFrequency() const
{
    return _inverterTargetFrequency;
}

bool HoymilesRadio_CMT::isConnected() const
{
    if (!_isInitialized) {
        return false;
    }
    return _radio->isChipConnected();
}

uint32_t HoymilesRadio_CMT::getMinFrequency()
{
    return MIN_FREQ;
}

uint32_t HoymilesRadio_CMT::getMaxFrequency()
{
    return MAX_FREQ;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt1()
{
    _packetSent = true;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt2()
{
    _packetReceived = true;
}

void HoymilesRadio_CMT::sendEsbPacket(CommandAbstract& cmd)
{
    cmd.incrementSendCount();

    cmd.setRouterAddress(DtuSerial().u64);

    _radio->stopListening();

    if (cmd.getDataPayload()[0] == 0x56) { // @todo(tbnobody) Bad hack to identify ChannelChange Command
        cmtSwitchDtuFreq(HOY_BOOT_FREQ);
    }

    Hoymiles.getMessageOutput()->printf("TX %s %.2f MHz --> ",
        cmd.getCommandName().c_str(), getFrequencyFromChannel(_radio->getChannel()) / 1000000.0);
    cmd.dumpDataPayload(Hoymiles.getMessageOutput());

    if (!_radio->write(cmd.getDataPayload(), cmd.getDataSize())) {
        Hoymiles.getMessageOutput()->println("TX SPI Timeout");
    }
    cmtSwitchDtuFreq(_inverterTargetFrequency);
    _radio->startListening();
    _busyFlag = true;
    _rxTimeout.set(cmd.getTimeout());
}

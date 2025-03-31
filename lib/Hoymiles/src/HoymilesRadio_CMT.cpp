// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "crc.h"
#include <FunctionalInterrupt.h>
#include <frozen/map.h>

constexpr CountryFrequencyDefinition_t make_value(FrequencyBand_t Band, uint32_t Freq_Legal_Min, uint32_t Freq_Legal_Max, uint32_t Freq_Default, uint32_t Freq_StartUp)
{
    // frequency can not be lower than actual initailized base freq + 250000
    uint32_t minFrequency = CMT2300A::getBaseFrequency(Band) + HoymilesRadio_CMT::getChannelWidth();

    // =923500, 0xFF does not work
    uint32_t maxFrequency = CMT2300A::getBaseFrequency(Band) + 0xFE * HoymilesRadio_CMT::getChannelWidth();

    CountryFrequencyDefinition_t v = { Band, minFrequency, maxFrequency, Freq_Legal_Min, Freq_Legal_Max, Freq_Default, Freq_StartUp };
    return v;
}

constexpr frozen::map<CountryModeId_t, CountryFrequencyDefinition_t, 3> countryDefinition = {
    { CountryModeId_t::MODE_EU, make_value(FrequencyBand_t::BAND_860, 863e6, 870e6, 865e6, 868e6) },
    { CountryModeId_t::MODE_US, make_value(FrequencyBand_t::BAND_900, 905e6, 925e6, 918e6, 915e6) },
    { CountryModeId_t::MODE_BR, make_value(FrequencyBand_t::BAND_900, 915e6, 928e6, 918e6, 915e6) },
};

uint32_t HoymilesRadio_CMT::getFrequencyFromChannel(const uint8_t channel) const
{
    return (_radio->getBaseFrequency() + channel * getChannelWidth());
}

uint8_t HoymilesRadio_CMT::getChannelFromFrequency(const uint32_t frequency) const
{
    if ((frequency % getChannelWidth()) != 0) {
        Hoymiles.getMessageOutput()->printf("%.3f MHz is not divisible by %" PRId32 " kHz!\r\n", frequency / 1000000.0, getChannelWidth());
        return 0xFF; // ERROR
    }
    if (frequency < getMinFrequency() || frequency > getMaxFrequency()) {
        Hoymiles.getMessageOutput()->printf("%.2f MHz is out of Hoymiles/CMT range! (%.2f MHz - %.2f MHz)\r\n",
            frequency / 1000000.0, getMinFrequency() / 1000000.0, getMaxFrequency() / 1000000.0);
        return 0xFF; // ERROR
    }
    if (frequency < countryDefinition.at(_countryMode).Freq_Legal_Min || frequency > countryDefinition.at(_countryMode).Freq_Legal_Max) {
        Hoymiles.getMessageOutput()->printf("!!! caution: %.2f MHz is out of region legal range! (%" PRId32 " - %" PRId32 " MHz)\r\n",
            frequency / 1000000.0,
            static_cast<uint32_t>(countryDefinition.at(_countryMode).Freq_Legal_Min / 1e6),
            static_cast<uint32_t>(countryDefinition.at(_countryMode).Freq_Legal_Max / 1e6));
    }

    return (frequency - _radio->getBaseFrequency()) / getChannelWidth(); // frequency to channel
}

std::vector<CountryFrequencyList_t> HoymilesRadio_CMT::getCountryFrequencyList() const
{
    std::vector<CountryFrequencyList_t> v;
    for (const auto& [key, value] : countryDefinition) {
        CountryFrequencyList_t s;
        s.mode = key;
        s.definition.Band = value.Band;
        s.definition.Freq_Default = value.Freq_Default;
        s.definition.Freq_StartUp = value.Freq_StartUp;
        s.definition.Freq_Min = value.Freq_Min;
        s.definition.Freq_Max = value.Freq_Max;
        s.definition.Freq_Legal_Max = value.Freq_Legal_Max;
        s.definition.Freq_Legal_Min = value.Freq_Legal_Min;

        v.push_back(s);
    }
    return v;
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

    setCountryMode(CountryModeId_t::MODE_EU);
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
            if (_rxBuffer.size() > FRAGMENT_BUFFER_SIZE) {
                Hoymiles.getMessageOutput()->println("CMT: Buffer full");
                _radio->flush_rx();
                continue;
            }

            fragment_t f;
            memset(f.fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
            f.len = std::min<uint8_t>(_radio->getDynamicPayloadSize(), MAX_RF_PAYLOAD_SIZE);
            f.channel = _radio->getChannel();
            f.rssi = _radio->getRssiDBm();
            f.wasReceived = false;
            f.mainCmd = 0x00;
            _radio->read(f.fragment, f.len);
            _rxBuffer.push(f);
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
                        Hoymiles.getMessageOutput()->printf("| %" PRId8 " dBm\r\n", f.rssi);

                        inv->addRxFragment(f.fragment, f.len, f.rssi);
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
        Hoymiles.getMessageOutput()->printf("CMT TX power set to %" PRId8 " dBm\r\n", paLevel);
    } else {
        Hoymiles.getMessageOutput()->printf("CMT TX power %" PRId8 " dBm is not defined! (min: -10 dBm, max: 20 dBm)\r\n", paLevel);
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

uint32_t HoymilesRadio_CMT::getMinFrequency() const
{
    return countryDefinition.at(_countryMode).Freq_Min;
}

uint32_t HoymilesRadio_CMT::getMaxFrequency() const
{
    return countryDefinition.at(_countryMode).Freq_Max;
}

CountryModeId_t HoymilesRadio_CMT::getCountryMode() const
{
    return _countryMode;
}

void HoymilesRadio_CMT::setCountryMode(const CountryModeId_t mode)
{
    _countryMode = mode;
    if (!_isInitialized) {
        return;
    }
    _radio->setFrequencyBand(countryDefinition.at(mode).Band);
}

uint32_t HoymilesRadio_CMT::getInvBootFrequency() const
{
    // Hoymiles boot/init frequency after power up inverter or connection lost for 15 min
    return countryDefinition.at(_countryMode).Freq_StartUp;
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
        cmtSwitchDtuFreq(getInvBootFrequency());
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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2026 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "HoymilesUtils.h"
#include "crc.h"
#include "inverters/InverterAbstract.h"
#include <FunctionalInterrupt.h>
#include <algorithm>
#include <esp_log.h>
#include <frozen/map.h>

#undef TAG
static const char* TAG = "hoymiles";

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
        ESP_LOGE(TAG, "%.3f MHz is not divisible by %" PRIu32 " kHz!", frequency / 1000000.0, getChannelWidth());
        return 0xFF; // ERROR
    }
    if (frequency < getMinFrequency() || frequency > getMaxFrequency()) {
        ESP_LOGE(TAG, "%.2f MHz is out of Hoymiles/CMT range! (%.2f MHz - %.2f MHz)",
            frequency / 1000000.0, getMinFrequency() / 1000000.0, getMaxFrequency() / 1000000.0);
        return 0xFF; // ERROR
    }
    if (frequency < countryDefinition.at(_countryMode).Freq_Legal_Min || frequency > countryDefinition.at(_countryMode).Freq_Legal_Max) {
        ESP_LOGE(TAG, "!!! caution: %.2f MHz is out of region legal range! (%" PRIu32 " - %" PRIu32 " MHz)",
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

uint32_t HoymilesRadio_CMT::getInvBootRingFrequency(const uint8_t startChannel) const
{
    const CountryFrequencyDefinition_t& def = countryDefinition.at(_countryMode);
    const uint8_t channel = (startChannel != 0) ? startChannel : ((def.Band == FrequencyBand_t::BAND_860) ? 33 : 60);
    return getFrequencyFromChannel(channel);
}

bool HoymilesRadio_CMT::getProbeFrequency(const uint16_t step, uint32_t& frequency, const uint8_t moduleStartChannel) const
{
    const CountryFrequencyDefinition_t& def = countryDefinition.at(_countryMode);

    const int16_t startChannel = (moduleStartChannel != 0) ? moduleStartChannel : ((def.Band == FrequencyBand_t::BAND_860) ? 33 : 60);

    const uint32_t base = _radio->getBaseFrequency();
    const int16_t minChannel = static_cast<int16_t>(std::max<int32_t>(1,
        (static_cast<int32_t>(def.Freq_Legal_Min) - static_cast<int32_t>(base) + static_cast<int32_t>(getChannelWidth()) - 1) / static_cast<int32_t>(getChannelWidth())));
    const int16_t maxChannel = static_cast<int16_t>(std::min<int32_t>(0xFE,
        (static_cast<int32_t>(def.Freq_Legal_Max) - static_cast<int32_t>(base)) / static_cast<int32_t>(getChannelWidth())));
    if (minChannel > maxChannel) {
        return false;
    }

    const int16_t preferred[] = {
        startChannel, static_cast<int16_t>(startChannel - 8),
        static_cast<int16_t>(startChannel + 1), static_cast<int16_t>(startChannel - 1),
        static_cast<int16_t>(startChannel - 7), static_cast<int16_t>(startChannel - 9)
    };

    int16_t candidates[sizeof(preferred) / sizeof(preferred[0])];
    uint16_t candidateCount = 0;
    for (const int16_t c : preferred) {
        if (c < minChannel || c > maxChannel) {
            continue;
        }
        bool duplicate = false;
        for (uint16_t i = 0; i < candidateCount; i++) {
            duplicate = duplicate || (candidates[i] == c);
        }
        if (!duplicate) {
            candidates[candidateCount++] = c;
        }
    }

    int16_t channel = -1;
    if (step < candidateCount) {
        channel = candidates[step];
    } else {
        uint16_t remaining = step - candidateCount;
        for (int16_t c = minChannel; c <= maxChannel; c++) {
            bool isCandidate = false;
            for (uint16_t i = 0; i < candidateCount; i++) {
                isCandidate = isCandidate || (candidates[i] == c);
            }
            if (isCandidate) {
                continue;
            }
            if (remaining == 0) {
                channel = c;
                break;
            }
            remaining--;
        }
    }

    if (channel < minChannel || channel > maxChannel) {
        return false;
    }

    frequency = getFrequencyFromChannel(static_cast<uint8_t>(channel));
    return true;
}

bool HoymilesRadio_CMT::rxRingContains(const uint8_t c) const
{
    return _rxHopCentreChannel >= 1 && c >= _rxHopCentreChannel - 1 && c <= _rxHopCentreChannel + 1;
}

uint8_t HoymilesRadio_CMT::rxRingNext(const uint8_t c) const
{
    if (!rxRingContains(c)) {
        return 0;
    }
    if (c == _rxHopCentreChannel - 1) {
        return _rxHopCentreChannel;
    }
    if (c == _rxHopCentreChannel) {
        return _rxHopCentreChannel + 1;
    }
    return _rxHopCentreChannel - 1;
}

void HoymilesRadio_CMT::startRxHopping(const uint32_t centreFrequency, const bool enabled)
{
    _rxHopHold = false;
    _rxHopActive = false;

    if (!enabled || !_rxHopEnabled) {
        return;
    }

    const uint8_t channel = getChannelFromFrequency(centreFrequency);
    if (channel < 1 || channel >= 0xFE) {
        return;
    }

    _rxHopCentreChannel = channel;
    _rxHopChannel = channel;
    _rxHopLastUs = micros();
    _rxHopActive = true;
}

void HoymilesRadio_CMT::rxHopStep()
{
    if (!_rxHopActive || !_busyFlag) {
        _rxHopActive = false;
        return;
    }

    const uint32_t now = micros();

    if (_packetReceived) {
        _rxHopHold = false;
        _rxHopLastUs = now;
        return;
    }

    switch (_radio->getRxState()) {
    case CmtRxState::Sync:
        if (!_rxHopHold) {
            _rxHopHold = true;
            _rxHopHoldUs = now;
        }
        if (now - _rxHopHoldUs < RX_HOP_HOLD_MAX_US) {
            return;
        }
        _rxHopHold = false;
        break;
    case CmtRxState::Preamble:
        _rxHopHold = false;
        _rxHopLastUs = now;
        return;
    default:
        _rxHopHold = false;
        if (now - _rxHopLastUs < RX_HOP_WAIT_US) {
            return;
        }
        break;
    }

    uint8_t channel = rxRingNext(_rxHopChannel);
    if (channel == 0) {
        channel = _rxHopCentreChannel;
    }

    _radio->setChannel(channel);
    _radio->startListening();
    _rxHopChannel = channel;
    _rxHopLastUs = micros();
}

void HoymilesRadio_CMT::init(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const int8_t pin_gpio2, const int8_t pin_gpio3)
{
    _dtuSerial.u64 = 0;

    _radio.reset(new CMT2300A(pin_sdio, pin_clk, pin_cs, pin_fcs));

    _radio->begin();

    setCountryMode(CountryModeId_t::MODE_EU);
    cmtSwitchDtuFreq(_inverterTargetFrequency); // start dtu at work freqency, for fast Rx if inverter is already on and frequency switched

    if (!_radio->isChipConnected()) {
        ESP_LOGE(TAG, "CMT2300A: Connection error!!");
        return;
    }
    ESP_LOGI(TAG, "CMT2300A: Connection successful");

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
        ESP_LOGV(TAG, "Interrupt received");
        while (_radio->available()) {
            if (_rxBuffer.size() > FRAGMENT_BUFFER_SIZE) {
                ESP_LOGE(TAG, "CMT2300A: Buffer full");
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

        _rxHopHold = false;
        _rxHopLastUs = micros();

    } else {
        rxHopStep();

        // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.front();
            if (checkFragmentCrc(f)) {

                const serial_u dtuId = convertSerialToRadioId(_dtuSerial);

                // The CMT RF module does not filter foreign packages by itself.
                // Has to be done manually here.
                if (memcmp(&f.fragment[5], &dtuId.b[1], 4) == 0) {

                    std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(f);

                    if (nullptr != inv) {
                        // Save packet in inverter rx buffer
                        ESP_LOGD(TAG, "RX %.2f MHz --> %s | %" PRId8 " dBm",
                            getFrequencyFromChannel(f.channel) / 1000000.0, HoymilesUtils::dumpArray(f.fragment, f.len).c_str(), f.rssi);

                        const bool ownsTransaction = _busyFlag && !isQueueEmpty()
                            && _commandQueue.front()->getTargetAddress() == inv->serial();
                        const uint8_t expectedMainCmd = ownsTransaction ? (_commandQueue.front()->getMainCmd() | 0x80) : 0;

                        inv->addRxFragment(f.fragment, f.len, f.rssi, expectedMainCmd);
                    } else {
                        ESP_LOGE(TAG, "Inverter Not found!");
                    }
                }

            } else {
                ESP_LOGW(TAG, "Frame kaputt"); // ;-)
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    handleReceivedPackage();
}

void HoymilesRadio_CMT::setPALevel(const int8_t paLevel)
{
    _paLevel = paLevel;
    if (!_isInitialized) {
        return;
    }

    if (_radio->setPALevel(paLevel)) {
        ESP_LOGI(TAG, "CMT TX power set to %" PRId8 " dBm", paLevel);
    } else {
        ESP_LOGE(TAG, "CMT TX power %" PRId8 " dBm is not defined! (min: -10 dBm, max: 20 dBm)", paLevel);
    }
}

void HoymilesRadio_CMT::setInverterTargetFrequency(const uint32_t frequency)
{
    _inverterTargetFrequency = frequency;
    if (!_isInitialized) {
        return;
    }
    // Switch channel while staying in RX — GoStby, change channel, GoRx
    _radio->stopListening();
    cmtSwitchDtuFreq(_inverterTargetFrequency);
    _radio->startListening();
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

    // setFrequencyBand() re-initializes the chip and thereby resets the
    // channel, the PA level and the RX state to the register bank defaults.
    // Restore them afterwards, otherwise the radio stays asleep on the band
    // base frequency with default TX power until the next config change.
    _radio->setFrequencyBand(countryDefinition.at(mode).Band);

    if (_inverterTargetFrequency >= getMinFrequency() && _inverterTargetFrequency <= getMaxFrequency()) {
        cmtSwitchDtuFreq(_inverterTargetFrequency);
    }
    _radio->setPALevel(_paLevel);
    _radio->startListening();
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

    uint32_t rxFrequency = cmd.getFrequency();
    if (rxFrequency == 0 && cmd.getInverter() != nullptr) {
        rxFrequency = cmd.getInverter()->CmtLink.frequency;
    }
    if (rxFrequency == 0) {
        rxFrequency = _inverterTargetFrequency;
    }

    uint32_t txFrequency = rxFrequency;
    if (cmd.usesBootFrequency()) {
        txFrequency = getInvBootFrequency();
        rxFrequency = getInvBootRingFrequency(cmd.getInverter() != nullptr ? cmd.getInverter()->CmtLink.startChannel : 0);
    }
    if (!cmtSwitchDtuFreq(txFrequency)) {
        txFrequency = _inverterTargetFrequency;
        rxFrequency = _inverterTargetFrequency;
        cmtSwitchDtuFreq(txFrequency);
    }

    ESP_LOGD(TAG, "TX %s %.2f MHz --> %s",
        cmd.getCommandName().c_str(), getFrequencyFromChannel(_radio->getChannel()) / 1000000.0, cmd.dumpDataPayload().c_str());

    if (!_radio->write(cmd.getDataPayload(), cmd.getDataSize())) {
        ESP_LOGE(TAG, "TX SPI Timeout");
    }
    cmtSwitchDtuFreq(rxFrequency);
    _radio->startListening();

    const bool knownSingleChannel = cmd.getInverter() != nullptr
        && cmd.getInverter()->RfInfo()->containsValidData()
        && cmd.getInverter()->RfInfo()->getModuleLine() == RfModuleLine::A;

    startRxHopping(rxFrequency, !knownSingleChannel);

    _busyFlag = true;
    _rxTimeout.set(cmd.getTimeout());
}

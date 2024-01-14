// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <Arduino.h>
#include <cmt2300wrapper.h>
#include <memory>
#include <queue>
#include <vector>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

#ifndef HOYMILES_CMT_WORK_FREQ
#define HOYMILES_CMT_WORK_FREQ 865000000
#endif

enum CountryModeId_t {
    MODE_EU,
    MODE_US,
    MODE_BR,
    CountryModeId_Max
};

struct CountryFrequencyDefinition_t {
    FrequencyBand_t Band;
    uint32_t Freq_Min;
    uint32_t Freq_Max;
    uint32_t Freq_Legal_Min;
    uint32_t Freq_Legal_Max;
    uint32_t Freq_Default;
    uint32_t Freq_StartUp;
};

struct CountryFrequencyList_t {
    CountryModeId_t mode;
    CountryFrequencyDefinition_t definition;
};

class HoymilesRadio_CMT : public HoymilesRadio {
public:
    void init(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const int8_t pin_gpio2, const int8_t pin_gpio3);
    void loop();
    void setPALevel(const int8_t paLevel);
    void setInverterTargetFrequency(const uint32_t frequency);
    uint32_t getInverterTargetFrequency() const;

    bool isConnected() const;

    uint32_t getMinFrequency() const;
    uint32_t getMaxFrequency() const;
    static constexpr uint32_t getChannelWidth()
    {
        return FH_OFFSET * CMT2300A_ONE_STEP_SIZE;
    }

    CountryModeId_t getCountryMode() const;
    void setCountryMode(const CountryModeId_t mode);

    uint32_t getInvBootFrequency() const;

    uint32_t getFrequencyFromChannel(const uint8_t channel) const;
    uint8_t getChannelFromFrequency(const uint32_t frequency) const;

    std::vector<CountryFrequencyList_t> getCountryFrequencyList() const;

private:
    void ARDUINO_ISR_ATTR handleInt1();
    void ARDUINO_ISR_ATTR handleInt2();

    void sendEsbPacket(CommandAbstract& cmd);

    std::unique_ptr<CMT2300A> _radio;

    volatile bool _packetReceived = false;
    volatile bool _packetSent = false;

    bool _gpio2_configured = false;
    bool _gpio3_configured = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _txTimeout;

    uint32_t _inverterTargetFrequency = HOYMILES_CMT_WORK_FREQ;

    bool cmtSwitchDtuFreq(const uint32_t to_frequency);

    CountryModeId_t _countryMode;
};

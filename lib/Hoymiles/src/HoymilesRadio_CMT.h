// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "CmtLinkState.h"
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

#ifndef HOYMILES_CMT_RX_HOP
#define HOYMILES_CMT_RX_HOP 1
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

    void setCaptureMode(const bool enabled);
    bool getCaptureMode() const;

    uint32_t getFrequencyFromChannel(const uint8_t channel) const;
    uint8_t getChannelFromFrequency(const uint32_t frequency) const;

    std::vector<CountryFrequencyList_t> getCountryFrequencyList() const;

    void setRxHopping(const bool enabled) { _rxHopEnabled = enabled; }
    bool getRxHopping() const { return _rxHopEnabled; }
    bool getProbeFrequency(const uint16_t step, uint32_t& frequency, const uint8_t startChannel = 0) const;
    uint32_t getInvBootRingFrequency(const uint8_t startChannel = 0) const;

private:
    void ARDUINO_ISR_ATTR handleInt1();
    void ARDUINO_ISR_ATTR handleInt2();

    void sendEsbPacket(CommandAbstract& cmd);

    // All received fragments are parsed before checking for early completion.
    virtual bool isRxBufferEmpty() const { return _rxBuffer.empty(); }

    std::unique_ptr<CMT2300A> _radio;

    bool _captureMode = false;

    volatile bool _packetReceived = false;
    volatile bool _packetSent = false;

    bool _gpio2_configured = false;
    bool _gpio3_configured = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _txTimeout;

    uint32_t _inverterTargetFrequency = HOYMILES_CMT_WORK_FREQ;
    int8_t _paLevel = 0;

    bool cmtSwitchDtuFreq(const uint32_t to_frequency);

    void resetLinkStates();
    void startRxHopping(const uint32_t centreFrequency, const bool enabled, const uint8_t firstChannel = 0, const uint32_t stayUs = 0);
    void rxHopStep();
    uint8_t rxRingNext(const uint8_t c) const;
    bool rxRingContains(const uint8_t c) const;
    void rxRingFollow(const uint8_t c, CmtLinkState& link, const bool ownsTransaction);

    static constexpr uint32_t RX_HOP_DWELL_US = 2000;
    static constexpr uint32_t RX_HOP_HOLD_MAX_US = 60000;
    static constexpr uint32_t RX_RING_SLOT_US = 48000;
    static constexpr uint8_t RX_RING_MAX_MISSES = 3;
    static constexpr uint32_t RX_FIRST_FRAME_STAY_US = 92000;

    bool _rxHopEnabled = HOYMILES_CMT_RX_HOP != 0;
    bool _rxHopActive = false;
    bool _rxHopHold = false;
    bool _rxRingFollowing = false;
    uint8_t _rxRingMisses = 0;
    uint8_t _rxHopCentreChannel = 0;
    uint8_t _rxHopChannel = 0;
    uint32_t _rxHopStayUntilUs = 0;
    uint32_t _rxHopLastUs = 0;
    uint32_t _rxHopHoldUs = 0;

    CountryModeId_t _countryMode = MODE_EU;

    // Channel hopping for capture mode
    uint8_t _captureChIdx = 0;
    uint32_t _captureLastHop = 0;
    static constexpr uint32_t CAPTURE_HOP_INTERVAL_MS = 50; // dwell time per channel (50ms × 29ch = ~1.5s sweep)
};

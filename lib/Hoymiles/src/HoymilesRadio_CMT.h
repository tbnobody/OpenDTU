// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <Arduino.h>
#include <cmt2300wrapper.h>
#include <memory>
#include <queue>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

#ifndef HOYMILES_CMT_WORK_FREQ
#define HOYMILES_CMT_WORK_FREQ 865000
#endif

class HoymilesRadio_CMT : public HoymilesRadio {
public:
    void init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, int8_t pin_gpio2, int8_t pin_gpio3);
    void loop();
    void setPALevel(int8_t paLevel);
    void setInverterTargetFrequency(uint32_t frequency);
    uint32_t getInverterTargetFrequency();

    bool isConnected();

    static uint32_t getMinFrequency();
    static uint32_t getMaxFrequency();

    static float getFrequencyFromChannel(const uint8_t channel);
    static uint8_t getChannelFromFrequency(const uint32_t freq_kHz);

private:
    void ARDUINO_ISR_ATTR handleInt1();
    void ARDUINO_ISR_ATTR handleInt2();

    void sendEsbPacket(CommandAbstract* cmd);

    std::unique_ptr<CMT2300A> _radio;

    volatile bool _packetReceived = false;
    volatile bool _packetSent = false;

    bool _gpio2_configured = false;
    bool _gpio3_configured = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _rxTimeout;
    TimeoutHelper _txTimeout;

    uint32_t _inverterTargetFrequency = HOYMILES_CMT_WORK_FREQ;

    bool cmtSwitchDtuFreq(const uint32_t to_freq_kHz);
};
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#pragma once

#include <cstdint>
#include <TaskSchedulerDeclarations.h>

class SMA_HMClass {
public:
    void init(Scheduler& scheduler, bool verboseLogging);
    void loop();
    void event1();
    float getPowerTotal() const { return _powerMeterPower; }
    float getPowerL1() const { return _powerMeterL1; }
    float getPowerL2() const { return _powerMeterL2; }
    float getPowerL3() const { return _powerMeterL3; }

private:
    void Soutput(int kanal, int index, int art, int tarif,
            char const* name, float value, uint32_t timestamp);

    uint8_t* decodeGroup(uint8_t* offset, uint16_t grouplen);

    bool _verboseLogging = false;
    float _powerMeterPower = 0.0;
    float _powerMeterL1 = 0.0;
    float _powerMeterL2 = 0.0;
    float _powerMeterL3 = 0.0;
    uint32_t _previousMillis = 0;
    uint32_t _serial = 0;
    Task _loopTask;
};

extern SMA_HMClass SMA_HM;

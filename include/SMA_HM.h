// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#pragma once

#include <cstdint>
#include <TaskSchedulerDeclarations.h>

class SMA_HMClass {
public:
    void init(Scheduler& scheduler);
    void loop();
    void event1();
    float getPowerTotal();
    float getPowerL1();
    float getPowerL2();
    float getPowerL3();
    uint32_t serial = 0;
private:
    uint32_t _lastUpdate = 0;
    float _powerMeterPower = 0.0;
    float _powerMeterL1 = 0.0;
    float _powerMeterL2 = 0.0;
    float _powerMeterL3 = 0.0;
    uint32_t previousMillis = 0;
    Task _loopTask;
};
extern SMA_HMClass SMA_HM;

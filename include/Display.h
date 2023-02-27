// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Display_helper.h"
#include <Arduino.h>

class DisplayClass {
public:
    void init(DisplayType_t _type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _reset, uint8_t _busy, uint8_t _dc);
    void loop();

    bool enablePowerSafe;
    bool enableScreensaver;
    uint8_t contrast;
    uint8_t rotation;
    uint16_t period = 10000; // Achtung, max 65535

private:
    DisplayType_t _display_type = DisplayType_t::None;
    uint32_t _lastDisplayUpdate = 0;
    time_t now = time(nullptr);
    uint16_t counterEPaper;
};

extern DisplayClass Display;
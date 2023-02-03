// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include "Display_helper.h"

class DisplayClass
{
public:
    void init(DisplayType_t type);
    void loop();

    bool enablePowerSafe;
    bool enableScreensaver;
    uint8_t contrast;
    uint16_t counterEPaper;
    uint16_t _period = 10000;
    uint32_t _lastDisplayUpdate = 0;
    time_t now = time(nullptr);

private:
    DisplayType_t _display_type = DisplayType_t::None;
};

extern DisplayClass Display;
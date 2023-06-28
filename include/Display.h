// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Display_helper.h"
#include "defaults.h"
#include <Arduino.h>

class DisplayClass {
public:
    void init(DisplayType_t _type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _reset, uint8_t _busy, uint8_t _dc);
    void loop();

    void setContrast(uint8_t contrast);
    void setOrientation(uint8_t rotation = DISPLAY_ROTATION);
    void setLanguage(uint8_t language);
    void setUpdatePeriod(uint16_t updatePeriod);
    void setEnablePowerSafe(bool display_PowerSafe);
    void setEnableScreensaver(bool display_ScreenSaver);

private:
    DisplayType_t _display_type = DisplayType_t::None;
    uint32_t _lastDisplayUpdate = 0;
    uint16_t _counterEPaper;
    uint16_t _settedUpdatePeriod = 10000; // Achtung, max 65535
};

extern DisplayClass Display;
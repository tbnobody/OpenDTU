// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "defaults.h"
#include <U8g2lib.h>

enum DisplayType_t {
    None,
    PCD8544,
    SSD1306,
    SH1106,
};

class DisplayGraphicClass {
public:
    DisplayGraphicClass();
    ~DisplayGraphicClass();

    void init(DisplayType_t type, uint8_t data, uint8_t clk, uint8_t cs, uint8_t reset);
    void loop();
    void setContrast(uint8_t contrast);
    void setOrientation(uint8_t rotation = DISPLAY_ROTATION);
    void setStartupDisplay();

    bool enablePowerSafe = true;
    bool enableScreensaver = true;

private:
    void printText(const char* text, uint8_t line);
    void calcLineHeights();
    void setFont(uint8_t line);

    U8G2* _display;

    DisplayType_t _display_type = DisplayType_t::None;
    uint8_t _mExtra;
    uint16_t _period = 1000;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _lastDisplayUpdate = 0;
    uint32_t _previousMillis = 0;
    char _fmtText[32];
    bool _isLarge = false;
    uint8_t _lineOffsets[5];
};

extern DisplayGraphicClass Display;
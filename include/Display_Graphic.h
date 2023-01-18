// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <U8g2lib.h>

class DisplayGraphicClass {
public:
    DisplayGraphicClass();
    ~DisplayGraphicClass();

    void init(uint8_t type);
    void loop();

    bool dispPowerSafe = true;
    bool dispLogo = true;
    uint8_t dispContrast = 60;

private:
    void printText(const char* text, uint8_t line);

    U8G2* _display;

    uint8_t _mExtra;
    uint8_t _display_type = 0;
    uint16_t _dispY = 0;
    uint16_t _period = 1000;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _lastDisplayUpdate = 0;
    uint32_t _previousMillis = 0;
    char _fmtText[32];
};

extern DisplayGraphicClass Display;
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Display_helper.h"
#include <U8g2lib.h>

class DisplayMonoClass
{
public:
    DisplayMonoClass();
    ~DisplayMonoClass();

    void init(DisplayType_t type, uint8_t clk, uint8_t data, uint8_t reset);
    void loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod);

    bool enablePowerSafe = true;
    bool enableScreensaver = true;
    bool showLogo = true;
    uint8_t contrast = 60;

private:
    void printText(const char *text, uint8_t line);

    U8G2 *_display;

    uint8_t _mExtra;
    uint16_t _dispY = 0;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _previousMillis = 0;
    char _fmtText[32];
};

extern DisplayMonoClass DisplayMono;
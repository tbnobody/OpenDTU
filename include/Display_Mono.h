// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Display_helper.h"
#include "defaults.h"
#include <U8g2lib.h>

class DisplayMonoClass {
public:
    DisplayMonoClass();
    ~DisplayMonoClass();

    void init(DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI);
    void loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod);

    void setContrast(uint8_t contrast);
    void setOrientation(uint8_t rotation);
    void setLanguage(uint8_t language);
    void setStartupDisplay();

    bool enablePowerSafe = true;
    bool enableScreensaver = true;

private:
    void calcLineHeights();
    void setFont(uint8_t line);
    void printText(const char* text, uint8_t line, uint8_t dispX);

    U8G2* _display;

    bool _mIsLarge = false;
    uint8_t mLoopCnt;
    uint32_t* mUtcTs;
    uint8_t mLineOffsets[5];

    uint16_t _dispY = 0;
    uint32_t _previousMillis = 0;
    uint8_t _display_language = DISPLAY_LANGUAGE;

    uint8_t _mExtra;
    uint16_t _mTimeout; // interval at which to power save (milliseconds)
    char _fmtText[32];

    const u8g2_cb_t* disp_rotation = U8G2_R2;
    uint8_t contrast = 60;
};

extern DisplayMonoClass DisplayMono;
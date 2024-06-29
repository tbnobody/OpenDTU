// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX, to use less code and ram
// #include <GFX.h>
// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 1

#include "Display.h"
#include <GxEPD2_3C.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <map>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "defaults.h"
#include "imagedata.h"

// GDEW027C44   2.7 " b/w/r 176x264, IL91874
// GDEH0154D67  1.54" b/w   200x200

class DisplayEPaperClass {
public:
    DisplayEPaperClass();
    ~DisplayEPaperClass();

    void init(Scheduler& scheduler, DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI);
    void loop();
    void fullRefresh();
    void setOrientation(uint8_t rotation);
    void setLanguage(uint8_t language);

private:
    void headlineIP();
    void actualPowerPaged(float _totalPower, float _totalYieldDay, float _totalYieldTotal, uint8_t _isprod);
    void lastUpdatePaged();

    Task _loopTask;

    const uint16_t _period = 10000;
    uint16_t _counterEPaper;

    bool _changed = false;
    char _fmtText[35];
    String _settedIP;
    uint8_t _headfootline = 16;
    uint8_t _displayRotation = DISPLAY_ROTATION;
    uint8_t _display_language = DISPLAY_LANGUAGE;
    GxEPD2_GFX* _display;
};

extern DisplayEPaperClass DisplayEPaper;
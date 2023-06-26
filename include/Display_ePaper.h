// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Includes ------------------------------------------------------------------*/
#include "Waveshare_Lib/epd1in54_V2.h"
#include "Waveshare_Lib/epdpaint.h"
#include <NetworkSettings.h>
#include <stdio.h>

#include "Display_helper.h"
#include "imagedata.h"

#define COLORED 0 // background color handler (dark)
#define UNCOLORED 1 // background color handler (light)

// GDEW027C44   2.7 " b/w/r 176x264, IL91874
// GDEH0154D67  1.54" b/w   200x200

class DisplayEPaperClass {
public:
    DisplayEPaperClass();
    ~DisplayEPaperClass();
    void fullRefresh();
    void init(DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI);
    void loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod);
    void setOrientation(uint8_t rotation);

private:
    void headlineIP();
    void actualPowerPaged(float _totalPower, float _totalYieldDay, float _totalYieldTotal, uint8_t _isprod);
    void lastUpdatePaged();

    Epd* epd; // initiate e-paper display [epd]
    Paint* paint;
    unsigned char image[1024]; // memory for display
    char _fmtText[35];
    uint8_t headfootline = 16;
    uint8_t actualPower_height = 30;
    uint16_t x;

    int initial_space = 5; // initial white/dark space at the top of the display
    int row_height = 24; // row height (based on text size)
};

extern DisplayEPaperClass DisplayEPaper;
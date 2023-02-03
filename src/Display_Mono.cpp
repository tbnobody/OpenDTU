// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_Mono.h"
#include <NetworkSettings.h>
#include <map>
#include <time.h>
#include "imagedata.h"

std::map<DisplayType_t, std::function<U8G2 *(uint8_t, uint8_t, uint8_t)>> mono_types = {
    {DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data)
     { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); }},
    {DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data)
     { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); }},
};

DisplayMonoClass::DisplayMonoClass()
{
}

DisplayMonoClass::~DisplayMonoClass()
{
    delete _display;
}

void DisplayMonoClass::init(DisplayType_t type, uint8_t clk, uint8_t data, uint8_t reset)
{
    if (type > DisplayType_t::None)
    {
        auto constructor = mono_types[type];
        _display = constructor(reset, clk, data);
        _display->begin();
    }
}

void DisplayMonoClass::printText(const char *text, uint8_t line)
{
    // pxMovement +x (0 - 6 px)
    uint8_t ex = enableScreensaver ? (_mExtra % 7) : 0;

    // set the font size based on the display size
    switch (line)
    {
    case 1:

        _display->setFont(u8g2_font_ncenB14_tr);
        break;
    case 4:

        _display->setFont(u8g2_font_5x8_tr);
        break;
    default:

        _display->setFont(u8g2_font_ncenB10_tr);
        break;
    }

    // get the font height, to calculate the textheight
    _dispY += (_display->getMaxCharHeight()) + 1;

    // calculate the starting position of the text
    uint16_t dispX;
    if (line == 1)
    {
        dispX = 20 + ex;
    }
    else
    {
        dispX = 5 + ex;
    }

    // draw the Text, on the calculated pos
    _display->drawStr(dispX, _dispY, text);
}

void DisplayMonoClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    _display->clearBuffer();

    // set Contrast of the Display to raise the lifetime
    _display->setContrast(contrast);

    //=====> Actual Production ==========
    if ((totalPower > 0) && (isprod > 0))
    {
        _display->setPowerSave(false);
        if (totalPower > 999)
        {
            snprintf(_fmtText, sizeof(_fmtText), "%2.2f kW", (totalPower / 1000));
        }
        else
        {
            snprintf(_fmtText, sizeof(_fmtText), "%3.0f W", totalPower);
        }
        printText(_fmtText, 1);
        _previousMillis = millis();
    }
    //<=======================

    //=====> Offline ===========
    else
    {
        printText("offline", 1);
        // check if it's time to enter power saving mode
        if (millis() - _previousMillis >= (_interval * 2))
        {
            _display->setPowerSave(enablePowerSafe);
        }
    }
    //<=======================

    //=====> Today & Total Production =======
    snprintf(_fmtText, sizeof(_fmtText), "today: %4.0f Wh", totalYieldDay);
    printText(_fmtText, 2);

    snprintf(_fmtText, sizeof(_fmtText), "total: %.1f kWh", totalYieldTotal);
    printText(_fmtText, 3);
    //<=======================

    //=====> IP or Date-Time ========
    if (!(_mExtra % 10) && NetworkSettings.localIP())
    {
        printText(NetworkSettings.localIP().toString().c_str(), 4);
    }
    else if (!(_mExtra % 5))
    {
        snprintf(_fmtText, sizeof(_fmtText), "#%d Inverter online", isprod);
        printText(_fmtText, 4);
    }
    else
    {
        time_t now = time(nullptr);
        strftime(_fmtText, sizeof(_fmtText), "%d.%m.%Y %H:%M", localtime(&now));
        printText(_fmtText, 4);
    }

    _display->sendBuffer();

    _dispY = 0;
    _mExtra++;
}

DisplayMonoClass DisplayMono;
// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_Mono.h"
#include "imagedata.h"
#include <NetworkSettings.h>
#include <map>
#include <time.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

std::map<DisplayType_t, std::function<U8G2*(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)>> mono_types = {
    { DisplayType_t::PCD8544, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc) { return new U8G2_PCD8544_84X48_F_4W_SW_SPI(U8G2_R2, clock, data, cs, dc, reset); } },
    { DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc) { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
};

DisplayMonoClass::DisplayMonoClass()
{
}

DisplayMonoClass::~DisplayMonoClass()
{
    delete _display;
}

void DisplayMonoClass::calcLineHeights()
{
    uint8_t yOff = 0;
    for (uint8_t i = 0; i < 4; i++) {
        setFont(i);
        yOff += (_display->getMaxCharHeight());
        mLineOffsets[i] = yOff;
    }
}

inline void DisplayMonoClass::setFont(uint8_t line)
{
    switch (line) {
    case 0:
        _display->setFont((_mIsLarge) ? u8g2_font_ncenB14_tr : u8g2_font_logisoso16_tr);
        break;
    case 3:
        _display->setFont(u8g2_font_5x8_tr);
        break;
    default:
        _display->setFont((_mIsLarge) ? u8g2_font_ncenB10_tr : u8g2_font_5x8_tr);
        break;
    }
}

void DisplayMonoClass::printText(const char* text, uint8_t line, uint8_t dispX = 5)
{
    if (!_mIsLarge) {
        dispX = (line == 0) ? 5 : 0;
    } else {
        dispX = (line == 0) ? 20 : 5;
    }
    setFont(line);

    dispX += enableScreensaver ? (_mExtra % 7) : 0;
    _display->drawStr(dispX, mLineOffsets[line], text);
}

void DisplayMonoClass::init(DisplayType_t _type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI)
{
    if (_type > DisplayType_t::None) {
        auto constructor = mono_types[_type];
        _display = constructor(_RST, _SCK, _MOSI, _CS, _DC);
        _display->begin();
        _display->setDisplayRotation(disp_rotation);

        _mIsLarge = (_display->getWidth() > 100);
        calcLineHeights();

        _display->clearBuffer();
        if (contrast < 255) {
            _display->setContrast(contrast);
        }
        printText("OpenDTU!", 0);
        _display->sendBuffer();
    }
}

void DisplayMonoClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    _display->clearBuffer();

    // set Contrast of the Display to raise the lifetime
    if (contrast < 255) {
        _display->setContrast(contrast);
    }

    //=====> Actual Production ==========
    if ((totalPower > 0) && (isprod > 0)) {
        _display->setPowerSave(false);
        if (totalPower > 999) {
            snprintf(_fmtText, sizeof(_fmtText), "%2.2f kW", (totalPower / 1000));
        } else {
            snprintf(_fmtText, sizeof(_fmtText), "%3.0f W", totalPower);
        }
        printText(_fmtText, 0);
        _previousMillis = millis();
    }
    //<=======================

    //=====> Offline ===========
    else {
        printText("offline", 0);
        // check if it's time to enter power saving mode
        if (millis() - _previousMillis >= (_mTimeout * 2)) {
            _display->setPowerSave(enablePowerSafe);
        }
    }
    //<=======================

    //=====> Today & Total Production =======
    snprintf(_fmtText, sizeof(_fmtText), "today: %4.0f Wh", totalYieldDay);
    printText(_fmtText, 1);

    snprintf(_fmtText, sizeof(_fmtText), "total: %.1f kWh", totalYieldTotal);
    printText(_fmtText, 2);
    //<=======================

    //=====> IP or Date-Time ========
    if (!(_mExtra % 10) && NetworkSettings.localIP()) {
        printText(NetworkSettings.localIP().toString().c_str(), 3);
    } else if (!(_mExtra % 5)) {
        snprintf(_fmtText, sizeof(_fmtText), "#%d Inverter online", isprod);
        printText(_fmtText, 3);
    } else {
        time_t now = time(nullptr);
        strftime(_fmtText, sizeof(_fmtText), "%d.%m.%Y %H:%M", localtime(&now));
        printText(_fmtText, 3);
    }

    _display->sendBuffer();

    _dispY = 0;
    _mExtra++;
}

DisplayMonoClass DisplayMono;
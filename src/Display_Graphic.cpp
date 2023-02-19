// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_Graphic.h"
#include <Hoymiles.h>
#include <NetworkSettings.h>
#include <map>
#include <time.h>

static uint8_t bmp_logo[] PROGMEM = {
    B00000000, B00000000, // ................
    B11101100, B00110111, // ..##.######.##..
    B11101100, B00110111, // ..##.######.##..
    B11100000, B00000111, // .....######.....
    B11010000, B00001011, // ....#.####.#....
    B10011000, B00011001, // ...##..##..##...
    B10000000, B00000001, // .......##.......
    B00000000, B00000000, // ................
    B01111000, B00011110, // ...####..####...
    B11111100, B00111111, // ..############..
    B01111100, B00111110, // ..#####..#####..
    B00000000, B00000000, // ................
    B11111100, B00111111, // ..############..
    B11111110, B01111111, // .##############.
    B01111110, B01111110, // .######..######.
    B00000000, B00000000 // ................
};

static uint8_t bmp_arrow[] PROGMEM = {
    B00000000, B00011100, B00011100, B00001110, B00001110, B11111110, B01111111,
    B01110000, B01110000, B00110000, B00111000, B00011000, B01111111, B00111111,
    B00011110, B00001110, B00000110, B00000000, B00000000, B00000000, B00000000
};

std::map<DisplayType_t, std::function<U8G2*(uint8_t, uint8_t, uint8_t, uint8_t)>> display_types = {
    { DisplayType_t::PCD8544, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_PCD8544_84X48_F_4W_HW_SPI(U8G2_R0, cs, data, reset); } },
    { DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
};

DisplayGraphicClass::DisplayGraphicClass()
{
}

DisplayGraphicClass::~DisplayGraphicClass()
{
    delete _display;
}

void DisplayGraphicClass::init(DisplayType_t type, uint8_t data, uint8_t clk, uint8_t cs, uint8_t reset)
{
    _display_type = type;
    if (_display_type > DisplayType_t::None) {
        auto constructor = display_types[_display_type];
        _display = constructor(reset, clk, data, cs);
        _display->begin();
    }
}

void DisplayGraphicClass::printText(const char* text, uint8_t line)
{
    // get the width and height of the display
    uint16_t maxWidth = _display->getWidth();
    uint16_t maxHeight = _display->getHeight();

    // pxMovement +x (0 - 6 px)
    uint8_t ex = enableScreensaver ? (_mExtra % 7) : 0;

    // set the font size based on the display size
    switch (line) {
    case 1:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_ncenB14_tr); // large display
        } else {
            _display->setFont(u8g2_font_logisoso16_tr); // small display
        }
        break;
    case 4:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_5x8_tr); // large display
        } else {
            _display->setFont(u8g2_font_5x8_tr); // small display
        }
        break;
    default:
        if (maxWidth > 120 && maxHeight > 60) {
            _display->setFont(u8g2_font_ncenB10_tr); // large display
        } else {
            _display->setFont(u8g2_font_5x8_tr); // small display
        }
        break;
    }

    // get the font height, to calculate the textheight
    _dispY += (_display->getMaxCharHeight()) + 1;

    // calculate the starting position of the text
    uint16_t dispX;
    if (line == 1) {
        dispX = 20 + ex;
    } else {
        dispX = 5 + ex;
    }

    // draw the Text, on the calculated pos
    _display->drawStr(dispX, _dispY, text);
}

void DisplayGraphicClass::loop()
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    if ((millis() - _lastDisplayUpdate) > _period) {
        float totalPower = 0;
        float totalYieldDay = 0;
        float totalYieldTotal = 0;

        uint8_t isprod = 0;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr) {
                continue;
            }

            if (inv->isProducing()) {
                isprod++;
            }

            for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
                totalPower += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                totalYieldDay += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YD);
                totalYieldTotal += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_YT);
            }
        }

        _display->clearBuffer();

        // set Contrast of the Display to raise the lifetime
        _display->setContrast(contrast);

        //=====> Logo and Lighting ==========
        //   pxMovement +x (0 - 6 px)
        uint8_t ex = enableScreensaver ? (_mExtra % 7) : 0;
        if (isprod > 0) {
            _display->drawXBMP(5 + ex, 1, 8, 17, bmp_arrow);
            if (showLogo) {
                _display->drawXBMP(_display->getWidth() - 24 + ex, 2, 16, 16, bmp_logo);
            }
        }
        //<=======================

        //=====> Actual Production ==========
        if ((totalPower > 0) && (isprod > 0)) {
            _display->setPowerSave(false);
            if (totalPower > 999) {
                snprintf(_fmtText, sizeof(_fmtText), "%2.1f kW", (totalPower / 1000));
            } else {
                snprintf(_fmtText, sizeof(_fmtText), "%3.0f W", totalPower);
            }
            printText(_fmtText, 1);
            _previousMillis = millis();
        }
        //<=======================

        //=====> Offline ===========
        else {
            printText("offline", 1);
            // check if it's time to enter power saving mode
            if (millis() - _previousMillis >= (_interval * 2)) {
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
        if (!(_mExtra % 10) && NetworkSettings.localIP()) {
            printText(NetworkSettings.localIP().toString().c_str(), 4);
        } else {
            // Get current time
            time_t now = time(nullptr);
            strftime(_fmtText, sizeof(_fmtText), "%a %d.%m.%Y %H:%M", localtime(&now));
            printText(_fmtText, 4);
        }
        _display->sendBuffer();

        _dispY = 0;
        _mExtra++;
        _lastDisplayUpdate = millis();
    }
}

DisplayGraphicClass Display;
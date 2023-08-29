// SPDX-License-Identifier: GPL-2.0-or-later
#include "Display_Graphic.h"
#include "Datastore.h"
#include <NetworkSettings.h>
#include <map>
#include <time.h>

std::map<DisplayType_t, std::function<U8G2*(uint8_t, uint8_t, uint8_t, uint8_t)>> display_types = {
    { DisplayType_t::PCD8544, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_PCD8544_84X48_F_4W_HW_SPI(U8G2_R0, cs, data, reset); } },
    { DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
};

// Language defintion, respect order in languages[] and translation lists
#define I18N_LOCALE_EN 0
#define I18N_LOCALE_DE 1
#define I18N_LOCALE_FR 2

// Languages supported. Note: the order is important and must match locale_translations.h
const uint8_t languages[] = {
    I18N_LOCALE_EN,
    I18N_LOCALE_DE,
    I18N_LOCALE_FR
};

static const char* const i18n_offline[] = { "Offline", "Offline", "Offline" };
static const char* const i18n_current_power_w[] = { "%3.0f W", "%3.0f W", "%3.0f W" };
static const char* const i18n_current_power_kw[] = { "%2.1f kW", "%2.1f kW", "%2.1f kW" };
static const char* const i18n_yield_today_wh[] = { "today: %4.0f Wh", "Heute: %4.0f Wh", "auj.: %4.0f Wh" };
static const char* const i18n_yield_total_kwh[] = { "total: %.1f kWh", "Ges.: %.1f kWh", "total: %.1f kWh" };
static const char* const i18n_date_format[] = { "%m/%d/%Y %H:%M", "%d.%m.%Y %H:%M", "%d/%m/%Y %H:%M" };

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
        setContrast(DISPLAY_CONTRAST);
        setStatus(true);
    }
}

void DisplayGraphicClass::calcLineHeights()
{
    uint8_t yOff = 0;
    for (uint8_t i = 0; i < 4; i++) {
        setFont(i);
        yOff += (_display->getMaxCharHeight());
        _lineOffsets[i] = yOff;
    }
}

void DisplayGraphicClass::setFont(uint8_t line)
{
    switch (line) {
    case 0:
        _display->setFont((_isLarge) ? u8g2_font_ncenB14_tr : u8g2_font_logisoso16_tr);
        break;
    case 3:
        _display->setFont(u8g2_font_5x8_tr);
        break;
    default:
        _display->setFont((_isLarge) ? u8g2_font_ncenB10_tr : u8g2_font_5x8_tr);
        break;
    }
}

void DisplayGraphicClass::printText(const char* text, uint8_t line)
{
    uint8_t dispX;
    if (!_isLarge) {
        dispX = (line == 0) ? 5 : 0;
    } else {
        dispX = (line == 0) ? 20 : 5;
    }
    setFont(line);

    dispX += enableScreensaver ? (_mExtra % 7) : 0;
    _display->drawStr(dispX, _lineOffsets[line], text);
}

void DisplayGraphicClass::setOrientation(uint8_t rotation)
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    switch (rotation) {
    case 0:
        _display->setDisplayRotation(U8G2_R0);
        break;
    case 1:
        _display->setDisplayRotation(U8G2_R1);
        break;
    case 2:
        _display->setDisplayRotation(U8G2_R2);
        break;
    case 3:
        _display->setDisplayRotation(U8G2_R3);
        break;
    }

    _isLarge = (_display->getWidth() > 100);
    calcLineHeights();
}

void DisplayGraphicClass::setLanguage(uint8_t language)
{
    _display_language = language < sizeof(languages) / sizeof(languages[0]) ? language : DISPLAY_LANGUAGE;
}

void DisplayGraphicClass::setStartupDisplay()
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    _display->clearBuffer();
    printText("OpenDTU!", 0);
    _display->sendBuffer();
}

void DisplayGraphicClass::loop()
{
    if (_display_type == DisplayType_t::None) {
        return;
    }

    if ((millis() - _lastDisplayUpdate) > _period) {

        _display->clearBuffer();
        bool displayPowerSave = false;

        //=====> Actual Production ==========
        if (Datastore.getIsAtLeastOneReachable()) {
            displayPowerSave = false;
            if (Datastore.getTotalAcPowerEnabled() > 999) {
                snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_kw[_display_language], (Datastore.getTotalAcPowerEnabled() / 1000));
            } else {
                snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_w[_display_language], Datastore.getTotalAcPowerEnabled());
            }
            printText(_fmtText, 0);
            _previousMillis = millis();
        }
        //<=======================

        //=====> Offline ===========
        else {
            printText(i18n_offline[_display_language], 0);
            // check if it's time to enter power saving mode
            if (millis() - _previousMillis >= (_interval * 2)) {
                displayPowerSave = enablePowerSafe;
            }
        }
        //<=======================

        //=====> Today & Total Production =======
        snprintf(_fmtText, sizeof(_fmtText), i18n_yield_today_wh[_display_language], Datastore.getTotalAcYieldDayEnabled());
        printText(_fmtText, 1);

        snprintf(_fmtText, sizeof(_fmtText), i18n_yield_total_kwh[_display_language], Datastore.getTotalAcYieldTotalEnabled());
        printText(_fmtText, 2);
        //<=======================

        //=====> IP or Date-Time ========
        if (!(_mExtra % 10) && NetworkSettings.localIP()) {
            printText(NetworkSettings.localIP().toString().c_str(), 3);
        } else {
            // Get current time
            time_t now = time(nullptr);
            strftime(_fmtText, sizeof(_fmtText), i18n_date_format[_display_language], localtime(&now));
            printText(_fmtText, 3);
        }
        _display->sendBuffer();

        _mExtra++;
        _lastDisplayUpdate = millis();

        if (!_displayTurnedOn) {
            displayPowerSave = true;
        }

        _display->setPowerSave(displayPowerSave);
    }
}

void DisplayGraphicClass::setContrast(uint8_t contrast)
{
    if (_display_type == DisplayType_t::None) {
        return;
    }
    _display->setContrast(contrast * 2.55f);
}

void DisplayGraphicClass::setStatus(bool turnOn)
{
    _displayTurnedOn = turnOn;
}

DisplayGraphicClass Display;
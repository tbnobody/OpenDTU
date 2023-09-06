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
    { DisplayType_t::SSD1309, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc) { return new U8G2_SSD1309_128X64_NONAME0_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs, uint8_t dc) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
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

static const char* const i18n_offline[] = { "offline", "Offline", "hors ligne" };
static const char* const i18n_online[] = { "#%d Inverter online", "#%d Wechselrichter online", "#%d Onduleur en ligne" };
static const char* const i18n_current_power_w[] = { "%3.0f W", "%3.0f W", "%3.0f W" };
static const char* const i18n_current_power_kw[] = { "%2.1f kW", "%2.1f kW", "%2.1f kW" };
static const char* const i18n_yield_today_wh[] = { "today: %4.0f Wh", "Heute: %4.0f Wh", "auj.: %4.0f Wh" };
static const char* const i18n_yield_total_kwh[] = { "total: %.1f kWh", "Ges.: %.1f kWh", "total: %.1f kWh" };
static const char* const i18n_date_format[] = { "%m/%d/%Y %H:%M", "%d.%m.%Y %H:%M", "%d/%m/%Y %H:%M" };

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

void DisplayMonoClass::setOrientation(uint8_t rotation)
{
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

    _mIsLarge = (_display->getWidth() > 100);
    calcLineHeights();
}

void DisplayMonoClass::setLanguage(uint8_t language)
{
    _display_language = language < sizeof(languages) / sizeof(languages[0]) ? language : DISPLAY_LANGUAGE;
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
        if (totalPower > 9999) {
            snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_kw[_display_language], (totalPower / 1000));
        } else {
            snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_w[_display_language], totalPower);
        }
        printText(_fmtText, 0);
        _previousMillis = millis();
    }
    //<=======================

    //=====> Offline ===========
    else {
        printText(i18n_offline[_display_language], 0);
        // check if it's time to enter power saving mode
        if (millis() - _previousMillis >= (_mTimeout * 2)) {
            _display->setPowerSave(enablePowerSafe);
        }
    }
    //<=======================

    //=====> Today & Total Production =======
    snprintf(_fmtText, sizeof(_fmtText), i18n_yield_today_wh[_display_language], totalYieldDay);
    printText(_fmtText, 1);

    snprintf(_fmtText, sizeof(_fmtText), i18n_yield_total_kwh[_display_language], totalYieldTotal);
    printText(_fmtText, 2);
    //<=======================

    //=====> IP or Date-Time ========
    if (!(_mExtra % 10) && NetworkSettings.localIP()) {
        printText(NetworkSettings.localIP().toString().c_str(), 3);
    } else if (!(_mExtra % 5)) {
        snprintf(_fmtText, sizeof(_fmtText), i18n_online[_display_language], isprod);
        printText(_fmtText, 3);
    } else {
        time_t now = time(nullptr);
        strftime(_fmtText, sizeof(_fmtText), i18n_date_format[_display_language], localtime(&now));
        printText(_fmtText, 3);
    }

    _display->sendBuffer();

    _dispY = 0;
    _mExtra++;
}

void DisplayMonoClass::setContrast(uint8_t contrast)
{
    _display->setContrast(contrast * 2.55f);
}

DisplayMonoClass DisplayMono;
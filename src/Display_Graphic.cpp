// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "Display_Graphic.h"
#include "Datastore.h"
#include <NetworkSettings.h>
#include <map>
#include <time.h>

std::map<DisplayType_t, std::function<U8G2*(uint8_t, uint8_t, uint8_t, uint8_t)>> display_types = {
    { DisplayType_t::PCD8544, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_PCD8544_84X48_F_4W_HW_SPI(U8G2_R0, cs, data, reset); } },
    { DisplayType_t::SSD1306, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SH1106, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, reset, clock, data); } },
    { DisplayType_t::SSD1309, [](uint8_t reset, uint8_t clock, uint8_t data, uint8_t cs) { return new U8G2_SSD1309_128X64_NONAME0_F_HW_I2C(U8G2_R0, reset, clock, data); } },
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
static const char* const i18n_current_power_w[] = { "%.0f W", "%.0f W", "%.0f W" };
static const char* const i18n_current_power_kw[] = { "%.1f kW", "%.1f kW", "%.1f kW" };
static const char* const i18n_yield_today_wh[] = { "today: %4.0f Wh", "Heute: %4.0f Wh", "auj.: %4.0f Wh" };
static const char* const i18n_yield_total_kwh[] = { "total: %.1f kWh", "Ges.: %.1f kWh", "total: %.1f kWh" };
static const char* const i18n_date_format[] = { "%m/%d/%Y %H:%M", "%d.%m.%Y %H:%M", "%d/%m/%Y %H:%M" };

DisplayGraphicClass::DisplayGraphicClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DisplayGraphicClass::loop, this))
{
}

DisplayGraphicClass::~DisplayGraphicClass()
{
    delete _display;
}

void DisplayGraphicClass::init(Scheduler& scheduler, const DisplayType_t type, const uint8_t data, const uint8_t clk, const uint8_t cs, const uint8_t reset)
{
    _display_type = type;
    if (isValidDisplay()) {
        auto constructor = display_types[_display_type];
        _display = constructor(reset, clk, data, cs);
        _display->begin();
        setContrast(DISPLAY_CONTRAST);
        setStatus(true);
        _diagram.init(scheduler, _display);

        scheduler.addTask(_loopTask);
        _loopTask.setInterval(_period);
        _loopTask.enable();
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

void DisplayGraphicClass::setFont(const uint8_t line)
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

bool DisplayGraphicClass::isValidDisplay()
{
    return _display_type > DisplayType_t::None && _display_type < DisplayType_Max;
}

void DisplayGraphicClass::printText(const char* text, const uint8_t line)
{
    setFont(line);

    uint8_t dispX;
    if (!_isLarge) {
        dispX = (line == 0) ? 5 : 0;
    } else {
        switch (line) {
        case 0:
            if (_diagram_mode == DiagramMode_t::Small) {
                // Center between left border and diagram
                dispX = (CHART_POSX - _display->getStrWidth(text)) / 2;
            } else {
                // Center on screen
                dispX = (_display->getDisplayWidth() - _display->getStrWidth(text)) / 2;
            }
            break;
        case 3:
            // Center on screen
            dispX = (_display->getDisplayWidth() - _display->getStrWidth(text)) / 2;
            break;
        default:
            dispX = 5;
            break;
        }
    }

    dispX += enableScreensaver ? (_mExtra % 7) : 0;
    _display->drawStr(dispX, _lineOffsets[line], text);
}

void DisplayGraphicClass::setOrientation(const uint8_t rotation)
{
    if (!isValidDisplay()) {
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

void DisplayGraphicClass::setLanguage(const uint8_t language)
{
    _display_language = language < sizeof(languages) / sizeof(languages[0]) ? language : DISPLAY_LANGUAGE;
}

void DisplayGraphicClass::setDiagramMode(DiagramMode_t mode)
{
    if (mode < DiagramMode_t::DisplayMode_Max) {
        _diagram_mode = mode;
    }
}

void DisplayGraphicClass::setStartupDisplay()
{
    if (!isValidDisplay()) {
        return;
    }

    _display->clearBuffer();
    printText("OpenDTU!", 0);
    _display->sendBuffer();
}

DisplayGraphicDiagramClass& DisplayGraphicClass::Diagram()
{
    return _diagram;
}

void DisplayGraphicClass::loop()
{
    _loopTask.setInterval(_period);

    _display->clearBuffer();
    bool displayPowerSave = false;
    bool showText = true;

    //=====> Actual Production ==========
    if (Datastore.getIsAtLeastOneReachable()) {
        displayPowerSave = false;
        if (_isLarge) {
            uint8_t screenSaverOffsetX = enableScreensaver ? (_mExtra % 7) : 0;
            switch (_diagram_mode) {
            case DiagramMode_t::Small:
                _diagram.redraw(screenSaverOffsetX, CHART_POSX, CHART_POSY, CHART_WIDTH, CHART_HEIGHT, false);
                break;
            case DiagramMode_t::Fullscreen:
                // Every 10 seconds
                if (_mExtra % (10 * 2) < 10) {
                    _diagram.redraw(screenSaverOffsetX, 10, 0, _display->getDisplayWidth() - 12, _display->getDisplayHeight() - 3, true);
                    showText = false;
                }
                break;
            default:
                break;
            }
        }
        if (showText) {
            const float watts = Datastore.getTotalAcPowerEnabled();
            if (watts > 999) {
                snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_kw[_display_language], watts / 1000);
            } else {
                snprintf(_fmtText, sizeof(_fmtText), i18n_current_power_w[_display_language], watts);
            }
            printText(_fmtText, 0);
        }
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

    if (showText) {
        //=====> Today & Total Production =======
        snprintf(_fmtText, sizeof(_fmtText), i18n_yield_today_wh[_display_language], Datastore.getTotalAcYieldDayEnabled());
        printText(_fmtText, 1);

        snprintf(_fmtText, sizeof(_fmtText), i18n_yield_total_kwh[_display_language], Datastore.getTotalAcYieldTotalEnabled());
        printText(_fmtText, 2);
        //<=======================

        //=====> IP or Date-Time ========
        // Change every 3 seconds
        if (!(_mExtra % (3 * 2) < 3) && NetworkSettings.localIP()) {
            printText(NetworkSettings.localIP().toString().c_str(), 3);
        } else {
            // Get current time
            time_t now = time(nullptr);
            strftime(_fmtText, sizeof(_fmtText), i18n_date_format[_display_language], localtime(&now));
            printText(_fmtText, 3);
        }
    }

    _display->sendBuffer();

    _mExtra++;

    if (!_displayTurnedOn) {
        displayPowerSave = true;
    }

    _display->setPowerSave(displayPowerSave);
}

void DisplayGraphicClass::setContrast(const uint8_t contrast)
{
    if (!isValidDisplay()) {
        return;
    }
    _display->setContrast(contrast * 2.55f);
}

void DisplayGraphicClass::setStatus(const bool turnOn)
{
    _displayTurnedOn = turnOn;
}

DisplayGraphicClass Display;

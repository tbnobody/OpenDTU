// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Display_Graphic_Diagram.h"
#include "defaults.h"
#include <TaskSchedulerDeclarations.h>
#include <U8g2lib.h>

enum DisplayType_t {
    None,
    PCD8544,
    SSD1306,
    SH1106,
};

class DisplayGraphicClass {
public:
    DisplayGraphicClass();
    ~DisplayGraphicClass();

    void init(Scheduler& scheduler, const DisplayType_t type, const uint8_t data, const uint8_t clk, const uint8_t cs, const uint8_t reset);
    void setContrast(const uint8_t contrast);
    void setStatus(const bool turnOn);
    void setOrientation(const uint8_t rotation = DISPLAY_ROTATION);
    void setLanguage(const uint8_t language);
    void setStartupDisplay();

    DisplayGraphicDiagramClass& Diagram();

    bool enablePowerSafe = true;
    bool enableScreensaver = true;

private:
    void loop();
    void printText(const char* text, const uint8_t line);
    void calcLineHeights();
    void setFont(const uint8_t line);

    Task _loopTask;

    U8G2* _display;
    DisplayGraphicDiagramClass _diagram;

    bool _displayTurnedOn;

    DisplayType_t _display_type = DisplayType_t::None;
    uint8_t _display_language = DISPLAY_LANGUAGE;
    uint8_t _mExtra;
    uint16_t _period = 1000;
    uint16_t _interval = 60000; // interval at which to power save (milliseconds)
    uint32_t _previousMillis = 0;
    char _fmtText[32];
    bool _isLarge = false;
    uint8_t _lineOffsets[5];
};

extern DisplayGraphicClass Display;
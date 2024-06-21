// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "defaults.h"
#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

enum DisplayType_t {
    None,
    PCD8544,
    SSD1306,
    SH1106,
    SSD1309,
    ST7567_GM12864I_59N,
    ePaper154,
    DisplayType_Max,
};

enum DiagramMode_t {
    Off,
    Small,
    Fullscreen,
    DisplayMode_Max,
};

class DisplayClass {
public:
    void init(Scheduler& scheduler, DisplayType_t _type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _reset, uint8_t _busy, uint8_t _dc);

    void setContrast(uint8_t contrast);
    void setStatus(bool turnOn);
    void setOrientation(uint8_t rotation = DISPLAY_ROTATION);
    void setLanguage(uint8_t language);
    void setUpdatePeriod(uint16_t updatePeriod);
    void setEnablePowerSafe(bool display_PowerSafe);
    void setEnableScreensaver(bool display_ScreenSaver);
    void setDiagramMode(DiagramMode_t mode);
    void setStartupDisplay();
    void DiagramUpdatePeriod();

private:
    bool _displayTurnedOn;

    DisplayType_t _display_type = DisplayType_t::None;
    uint32_t _lastDisplayUpdate = 0;
    uint16_t _counterEPaper;
    uint16_t _settedUpdatePeriod = 10000; // Achtung, max 65535
};

extern DisplayClass Display;
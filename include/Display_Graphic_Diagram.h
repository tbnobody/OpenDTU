// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <U8g2lib.h>
#include <array>

#define CHART_HEIGHT 20 // chart area hight in pixels
#define CHART_WIDTH 47 // chart area width in pixels
#define DIAG_POSX 80 // position were Diag is drawn at
#define DIAG_POSY 0

class DisplayGraphicDiagramClass {
public:
    DisplayGraphicDiagramClass();

    void init(Scheduler& scheduler, U8G2* display);
    void redraw();

    void updatePeriod();

private:
    void averageLoop();
    void dataPointLoop();

    static uint32_t getSecondsPerDot();

    Task _averageTask;
    Task _dataPointTask;

    U8G2* _display = nullptr;
    std::array<float, CHART_WIDTH> _graphValues = {};
    uint8_t _graphValuesCount = 0;

    float _iRunningAverage = 0;
    uint16_t _iRunningAverageCnt = 0;

    uint8_t _graphPosX = DIAG_POSX;
};
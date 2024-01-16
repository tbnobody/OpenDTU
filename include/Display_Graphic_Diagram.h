// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <U8g2lib.h>
#include <array>

#define MAX_DATAPOINTS 128

class DisplayGraphicDiagramClass {
public:
    DisplayGraphicDiagramClass();

    void init(Scheduler& scheduler, U8G2* display);
    void redraw(uint8_t screenSaverOffsetX, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height, bool isFullscreen);

    void updatePeriod();

private:
    void averageLoop();
    void dataPointLoop();

    uint32_t getSecondsPerDot();

    Task _averageTask;
    Task _dataPointTask;

    U8G2* _display = nullptr;
    std::array<float, MAX_DATAPOINTS> _graphValues = {};
    uint8_t _graphValuesCount = 0;

    uint8_t _chartWidth = MAX_DATAPOINTS;

    float _iRunningAverage = 0;
    uint16_t _iRunningAverageCnt = 0;
};

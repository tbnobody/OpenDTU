// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "Display_Graphic_Diagram.h"
#include "Configuration.h"
#include "Datastore.h"
#include <algorithm>

DisplayGraphicDiagramClass::DisplayGraphicDiagramClass()
{
}

void DisplayGraphicDiagramClass::init(Scheduler& scheduler, U8G2* display)
{
    _display = display;

    scheduler.addTask(_averageTask);
    _averageTask.setCallback(std::bind(&DisplayGraphicDiagramClass::averageLoop, this));
    _averageTask.setIterations(TASK_FOREVER);
    _averageTask.setInterval(1 * TASK_SECOND);
    _averageTask.enable();

    scheduler.addTask(_dataPointTask);
    _dataPointTask.setCallback(std::bind(&DisplayGraphicDiagramClass::dataPointLoop, this));
    _dataPointTask.setIterations(TASK_FOREVER);
    updatePeriod();
    _dataPointTask.enable();
}

void DisplayGraphicDiagramClass::averageLoop()
{
    const float currentWatts = Datastore.getTotalAcPowerEnabled(); // get the current AC production
    _iRunningAverage += currentWatts;
    _iRunningAverageCnt++;
}

void DisplayGraphicDiagramClass::dataPointLoop()
{
    if (_graphValuesCount >= std::size(_graphValues)) {
        for (uint8_t i = 0; i < std::size(_graphValues) - 1; i++) {
            _graphValues[i] = _graphValues[i + 1];
        }
        _graphValuesCount = std::size(_graphValues) - 1;
    }
    if (_iRunningAverageCnt != 0) {
        _graphValues[_graphValuesCount++] = _iRunningAverage / _iRunningAverageCnt;
        _iRunningAverage = 0;
        _iRunningAverageCnt = 0;
    }
}

uint32_t DisplayGraphicDiagramClass::getSecondsPerDot()
{
    return Configuration.get().Display.DiagramDuration / CHART_WIDTH;
}

void DisplayGraphicDiagramClass::updatePeriod()
{
    _dataPointTask.setInterval(getSecondsPerDot() * TASK_SECOND);
}

void DisplayGraphicDiagramClass::redraw(uint8_t screenSaverOffsetX)
{
    uint8_t __graphPosX = DIAG_POSX + ((screenSaverOffsetX > 3) ?1 :0); // screenSaverOffsetX expected to be in range 0..6
    uint8_t graphPosY = DIAG_POSY;

    // draw diagram axis
    _display->drawVLine(_graphPosX, graphPosY, CHART_HEIGHT);
    _display->drawHLine(_graphPosX, graphPosY + CHART_HEIGHT - 1, CHART_WIDTH);

    _display->drawLine(_graphPosX + 1, graphPosY + 1, _graphPosX + 2, graphPosY + 2); // UP-arrow
    _display->drawLine(_graphPosX - 2, graphPosY + 2, _graphPosX - 1, graphPosY + 1); // UP-arrow
    _display->drawLine(_graphPosX + CHART_WIDTH - 3, graphPosY + CHART_HEIGHT - 3, _graphPosX + CHART_WIDTH - 2, graphPosY + CHART_HEIGHT - 2); // LEFT-arrow
    _display->drawLine(_graphPosX + CHART_WIDTH - 3, graphPosY + CHART_HEIGHT + 1, _graphPosX + CHART_WIDTH - 2, graphPosY + CHART_HEIGHT); // LEFT-arrow

    // draw AC value
    _display->setFont(u8g2_font_tom_thumb_4x6_mr); // 4 pixels per char
    char fmtText[7];
    const float maxWatts = *std::max_element(_graphValues.begin(), _graphValues.end()) + 1200;
    if (maxWatts > 999) {
        snprintf(fmtText, sizeof(fmtText), "%2.1fkW", maxWatts / 1000);
    } else {
        snprintf(fmtText, sizeof(fmtText), "%dW", static_cast<uint16_t>(maxWatts));
    }
    const uint8_t textLength = strlen(fmtText);
    const uint8_t space_and_arrow_pixels = 2;
    _display->drawStr(_graphPosX - space_and_arrow_pixels - (textLength * 4), graphPosY + 5, fmtText);

    // draw chart
    const float scaleFactor = maxWatts / CHART_HEIGHT;
    uint8_t axisTick = 1;
    for (int i = 0; i < _graphValuesCount; i++) {
        if (scaleFactor > 0) {
            if (i == 0) {
                _display->drawPixel(_graphPosX + 1 + i, graphPosY + CHART_HEIGHT - ((_graphValues[i] / scaleFactor) + 0.5)); // + 0.5 to round mathematical
            } else {
                _display->drawLine(_graphPosX + i, graphPosY + CHART_HEIGHT - ((_graphValues[i - 1] / scaleFactor) + 0.5), _graphPosX + 1 + i, graphPosY + CHART_HEIGHT - ((_graphValues[i] / scaleFactor) + 0.5));
            }
        }

        // draw one tick per hour to the x-axis
        if (i * getSecondsPerDot() > (3600u * axisTick)) {
            _display->drawPixel(_graphPosX + 1 + i, graphPosY + CHART_HEIGHT);
            axisTick++;
        }
    }
}

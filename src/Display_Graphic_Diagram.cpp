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
    // screenSaverOffsetX expected to be in range 0..6
    const uint8_t graphPosX = DIAG_POSX + ((screenSaverOffsetX > 3) ? 1 : 0);
    const uint8_t graphPosY = DIAG_POSY + ((screenSaverOffsetX > 3) ? 1 : 0);

    const uint8_t horizontal_line_y = graphPosY + CHART_HEIGHT - 1;
    const uint8_t arrow_size = 2;

    // draw diagram axis
    _display->drawVLine(graphPosX, graphPosY, CHART_HEIGHT);
    _display->drawHLine(graphPosX, horizontal_line_y, CHART_WIDTH);

    // UP-arrow
    _display->drawLine(graphPosX, graphPosY, graphPosX + arrow_size, graphPosY + arrow_size);
    _display->drawLine(graphPosX, graphPosY, graphPosX - arrow_size, graphPosY + arrow_size);

    // LEFT-arrow
    _display->drawLine(graphPosX + CHART_WIDTH - 1, horizontal_line_y, graphPosX + CHART_WIDTH - 1 - arrow_size, horizontal_line_y - arrow_size);
    _display->drawLine(graphPosX + CHART_WIDTH - 1, horizontal_line_y, graphPosX + CHART_WIDTH - 1 - arrow_size, horizontal_line_y + arrow_size);

    // draw AC value
    // 4 pixels per char
    _display->setFont(u8g2_font_tom_thumb_4x6_mr);
    char fmtText[7];
    const float maxWatts = *std::max_element(_graphValues.begin(), _graphValues.end());
    if (maxWatts > 999) {
        snprintf(fmtText, sizeof(fmtText), "%2.1fkW", maxWatts / 1000);
    } else {
        snprintf(fmtText, sizeof(fmtText), "%dW", static_cast<uint16_t>(maxWatts));
    }
    const uint8_t textLength = strlen(fmtText);
    _display->drawStr(graphPosX - arrow_size - textLength * 4, graphPosY + 5, fmtText);

    // draw chart
    const float scaleFactor = maxWatts / CHART_HEIGHT;
    uint8_t axisTick = 1;
    for (uint8_t i = 1; i < _graphValuesCount; i++) {
        // draw one tick per hour to the x-axis
        if (i * getSecondsPerDot() > (3600u * axisTick)) {
            _display->drawPixel(graphPosX + 1 + i, graphPosY + CHART_HEIGHT);
            axisTick++;
        }

        if (scaleFactor == 0) {
            continue;
        }

        _display->drawLine(
            graphPosX + i - 1, horizontal_line_y - std::max<int16_t>(0, _graphValues[i - 1] / scaleFactor - 0.5),
            graphPosX + i, horizontal_line_y - std::max<int16_t>(0, _graphValues[i] / scaleFactor - 0.5));
    }
}

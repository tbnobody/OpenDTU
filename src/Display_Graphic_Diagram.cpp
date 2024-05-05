// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "Display_Graphic_Diagram.h"
#include "Configuration.h"
#include "Datastore.h"
#include "SunPosition.h"
#include <algorithm>

DisplayGraphicDiagramClass::DisplayGraphicDiagramClass()
    : _averageTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&DisplayGraphicDiagramClass::averageLoop, this))
    , _dataPointTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DisplayGraphicDiagramClass::dataPointLoop, this))
{
}

void DisplayGraphicDiagramClass::init(Scheduler& scheduler, U8G2* display)
{
    _display = display;

    scheduler.addTask(_averageTask);
    _averageTask.enable();

    scheduler.addTask(_dataPointTask);
    updatePeriod();
    _dataPointTask.enable();
}

void DisplayGraphicDiagramClass::averageLoop()
{
    const float currentWatts = Datastore.getTotalAcPowerEnabled(); // get the current AC production
    _iRunningAverage += currentWatts;
    _iRunningAverageCnt++;
    if(SunPosition.isDayPeriod()) {
        if(wasNight) {  //neuer tag
            _graphValuesCount = 0;
            wasNight = false;
        }
    } else {
        wasNight = true;
    }
    if(SunPosition.dayPeriodLength() == 0) { // noch keine Uhrzeit
        _graphValuesCount = 0;
    } else {
        if(SunPosition.dayPeriodLength() != _daySeconds) { // neue Tageslänge
            updatePeriod();
            _daySeconds = SunPosition.dayPeriodLength();
        }
    }
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
    return SunPosition.dayPeriodLength() / _chartWidth;
}

void DisplayGraphicDiagramClass::updatePeriod()
{
    //  Calculate seconds per datapoint
    _dataPointTask.setInterval(SunPosition.dayPeriodLength() * TASK_SECOND / MAX_DATAPOINTS);
}

void DisplayGraphicDiagramClass::redraw(uint8_t screenSaverOffsetX, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height, bool isFullscreen)
{
    _chartWidth = width;
    Serial.println(SunPosition.dayPeriodLength());

    // screenSaverOffsetX expected to be in range 0..6
    const uint8_t graphPosX = xPos + ((screenSaverOffsetX > 3) ? 1 : 0);
    const uint8_t graphPosY = yPos + ((screenSaverOffsetX > 3) ? 1 : 0);

    const uint8_t horizontal_line_y = graphPosY + height - 1;
    const uint8_t arrow_size = 2;

    const float maxWatts = Datastore.getTotalDcIrradiationInstalled();//*std::max_element(_graphValues.begin(), _graphValues.end());

    // draw diagram axis
    //_display->drawHLine(graphPosX, horizontal_line_y, width);
    if (!isFullscreen) {
        _display->drawVLine(graphPosX, graphPosY, height);

        // UP-arrow
        _display->drawLine(graphPosX, graphPosY, graphPosX + arrow_size, graphPosY + arrow_size);
        _display->drawLine(graphPosX, graphPosY, graphPosX - arrow_size, graphPosY + arrow_size);

        // LEFT-arrow
        _display->drawLine(graphPosX + width - 1, horizontal_line_y, graphPosX + width - 1 - arrow_size, horizontal_line_y - arrow_size);
        _display->drawLine(graphPosX + width - 1, horizontal_line_y, graphPosX + width - 1 - arrow_size, horizontal_line_y + arrow_size);

        // draw AC value
        char fmtText[7];
        if (maxWatts > 999) {
            snprintf(fmtText, sizeof(fmtText), "%2.1fkW", maxWatts / 1000);
        } else {
            snprintf(fmtText, sizeof(fmtText), "%dW", static_cast<uint16_t>(maxWatts));
        }
        _display->setFont(u8g2_font_tom_thumb_4x6_mr);
        _display->drawStr(graphPosX - arrow_size - _display->getStrWidth(fmtText), graphPosY + 5, fmtText);
    }
    else {
        float wattsToday = Datastore.getTotalAcYieldDayEnabled();
        char text[5];
        if (wattsToday > 999) {
            snprintf(text, 10, "%.0fkWh" , wattsToday / 1000);
        } else {
            snprintf(text, 10, "%.0fWh", wattsToday);
        }
        _display->setFont(u8g2_font_unifont_t_0_76);
        _display->drawUTF8(0, 16, text);
    }

    // draw chart
    const float scaleFactorY = maxWatts / static_cast<float>(height);
    const float scaleFactorX = static_cast<float>(MAX_DATAPOINTS) / static_cast<float>(_chartWidth);

    uint8_t xAxisTicks = 1;
    Serial.println(_graphValuesCount);
    for (uint8_t i = 0; i < _graphValuesCount; i++) {
        // draw one tick per hour to the x-axis
        if (!isFullscreen && i * getSecondsPerDot() > (3600u * xAxisTicks)) {
            _display->drawPixel((graphPosX + 1 + i) * scaleFactorX, graphPosY + height);
            xAxisTicks++;
        }

        if (scaleFactorY == 0 || scaleFactorX == 0) {
            continue;
        }

        _display->drawLine(
            graphPosX + i / scaleFactorX, horizontal_line_y - std::max<int16_t>(0, _graphValues[i] / scaleFactorY),
            graphPosX + i / scaleFactorX, horizontal_line_y);
    }
}

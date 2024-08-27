// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "Display_Graphic_Diagram.h"
#include "Configuration.h"
#include "Datastore.h"
#include <algorithm>
#include <Preferences.h>


// Expected time after a restart all inverters are ready
#define EXPECTED_SECS_INVERTERS_AVAILABLE_AFTER_RESTART 60


DisplayGraphicDiagramClass::DisplayGraphicDiagramClass()
    : _averageTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&DisplayGraphicDiagramClass::averageLoop, this))
    , _dataPointTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&DisplayGraphicDiagramClass::dataPointLoop, this))
{
}


void DisplayGraphicDiagramClass::init(Scheduler& scheduler, U8G2* display)
{
    bool delayUpdateTasks;

    _display = display;

    restoreGraphValuesAfterRestart(delayUpdateTasks);

    scheduler.addTask(_averageTask);
    if (delayUpdateTasks) {
        _averageTask.restartDelayed(EXPECTED_SECS_INVERTERS_AVAILABLE_AFTER_RESTART * TASK_SECOND);
    } else {
        _averageTask.enable();
    }

    scheduler.addTask(_dataPointTask);
    updatePeriod();
    if (delayUpdateTasks) {
        _dataPointTask.restartDelayed(EXPECTED_SECS_INVERTERS_AVAILABLE_AFTER_RESTART * TASK_SECOND);
    } else {
        _dataPointTask.enable();
    }
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
    return Configuration.get().Display.Diagram.Duration / _chartWidth;
}

void DisplayGraphicDiagramClass::updatePeriod()
{
    //  Calculate seconds per datapoint
    _dataPointTask.setInterval(Configuration.get().Display.Diagram.Duration * TASK_SECOND / MAX_DATAPOINTS);
}

void DisplayGraphicDiagramClass::redraw(uint8_t screenSaverOffsetX, uint8_t xPos, uint8_t yPos, uint8_t width, uint8_t height, bool isFullscreen)
{
    _chartWidth = width;

    // screenSaverOffsetX expected to be in range 0..6
    const uint8_t graphPosX = xPos + ((screenSaverOffsetX > 3) ? 1 : 0);
    const uint8_t graphPosY = yPos + ((screenSaverOffsetX > 3) ? 1 : 0);

    const uint8_t horizontal_line_y = graphPosY + height - 1;
    const uint8_t arrow_size = 2;

    // draw diagram axis
    _display->drawVLine(graphPosX, graphPosY, height);
    _display->drawHLine(graphPosX, horizontal_line_y, width);

    // UP-arrow
    _display->drawLine(graphPosX, graphPosY, graphPosX + arrow_size, graphPosY + arrow_size);
    _display->drawLine(graphPosX, graphPosY, graphPosX - arrow_size, graphPosY + arrow_size);

    // LEFT-arrow
    _display->drawLine(graphPosX + width - 1, horizontal_line_y, graphPosX + width - 1 - arrow_size, horizontal_line_y - arrow_size);
    _display->drawLine(graphPosX + width - 1, horizontal_line_y, graphPosX + width - 1 - arrow_size, horizontal_line_y + arrow_size);

    // draw AC value
    char fmtText[7];
    const float maxWatts = *std::max_element(_graphValues.begin(), _graphValues.end());
    if (maxWatts > 999) {
        snprintf(fmtText, sizeof(fmtText), "%2.1fkW", maxWatts / 1000);
    } else {
        snprintf(fmtText, sizeof(fmtText), "%dW", static_cast<uint16_t>(maxWatts));
    }

    if (isFullscreen) {
        _display->setFont(u8g2_font_5x8_tr);
        _display->setFontDirection(3);
        _display->drawStr(graphPosX - arrow_size, graphPosY + _display->getStrWidth(fmtText), fmtText);
        _display->setFontDirection(0);
    } else {
        // 4 pixels per char
        _display->setFont(u8g2_font_tom_thumb_4x6_mr);
        _display->drawStr(graphPosX - arrow_size - _display->getStrWidth(fmtText), graphPosY + 5, fmtText);
    }

    // draw chart
    const float scaleFactorY = maxWatts / static_cast<float>(height);
    const float scaleFactorX = static_cast<float>(MAX_DATAPOINTS) / static_cast<float>(_chartWidth);

    if (maxWatts > 0 && isFullscreen) {
        // draw y axis ticks
        const uint16_t yAxisWattPerTick = maxWatts <= 100 ? 10 : maxWatts <= 1000 ? 100
            : maxWatts < 5000                                                     ? 500
                                                                                  : 1000;
        const uint8_t yAxisTickSizePixel = height / (maxWatts / yAxisWattPerTick);

        for (int16_t tickYPos = graphPosY + height; tickYPos > graphPosY - arrow_size; tickYPos -= yAxisTickSizePixel) {
            _display->drawPixel(graphPosX - 1, tickYPos);
        }
    }

    uint8_t xAxisTicks = 1;
    for (uint8_t i = 1; i < _graphValuesCount; i++) {
        // draw one tick per hour to the x-axis
        if (i * getSecondsPerDot() > (3600u * xAxisTicks)) {
            _display->drawPixel((graphPosX + 1 + i) * scaleFactorX, graphPosY + height);
            xAxisTicks++;
        }

        if (scaleFactorY == 0 || scaleFactorX == 0) {
            continue;
        }

        _display->drawLine(
            graphPosX + (i - 1) / scaleFactorX, horizontal_line_y - std::max<int16_t>(0, _graphValues[i - 1] / scaleFactorY - 0.5),
            graphPosX + i / scaleFactorX, horizontal_line_y - std::max<int16_t>(0, _graphValues[i] / scaleFactorY - 0.5));
    }
}

void DisplayGraphicDiagramClass::backupGraphValuesBeforeRestart()
{
    auto prefs = Preferences();

    if (!prefs.begin("OPENDtuGraphVal")) {
        return;
    }
    _dataPointTask.disable();
    // TODO(stefan@obssys.com)
    // Find an easy way to wait till a possible current execution of _dataPointTask has ended
    // Or: Use a mutex here and in dataPointLoop()
    // For now: I assume restarting is very rare and we validate everything in restoreGraphValuesAfterRestart()
    prefs.putUChar("count", _graphValuesCount);
    prefs.putBytes("vars", &_graphValues[0], sizeof(_graphValues));
    prefs.end();
}

void DisplayGraphicDiagramClass::restoreGraphValuesAfterRestart(bool& delayUpdateTasks)
{
    auto prefs = Preferences();

    delayUpdateTasks = false;

    if (!prefs.begin("OPENDtuGraphVal")) {
        return;
    }
    _graphValuesCount = prefs.getUChar("count", _graphValuesCount);
    if (_graphValuesCount >= std::size(_graphValues)) {
        _graphValuesCount = std::size(_graphValues) - 1;
    }
    prefs.getBytes("vars", &_graphValues[0], sizeof(_graphValues));
    prefs.clear(); // clear - so only after a Utils::restartDtu() the variables are available
    prefs.end();

    // Check if it was a restart due software reset
    if (esp_reset_reason() != ESP_RST_SW) {
        _graphValuesCount = 0;
        return;
    }

    // Delaying the data collection makes no sense if a dot is less than the time we need
    // to query all the inverters after a reboot.
    // At 60secs this equals a diagram period of ~7680sec (2h8m) on a 128 dot display
     _chartWidth = _display->getDisplayWidth(); // assume diagram use the whole display
     if (getSecondsPerDot() <= EXPECTED_SECS_INVERTERS_AVAILABLE_AFTER_RESTART) {
        return;
     }

    delayUpdateTasks = (_graphValuesCount != 0);
}

void DisplayGraphicDiagramClass::prepareDtuRestart()
{
    backupGraphValuesBeforeRestart();
}

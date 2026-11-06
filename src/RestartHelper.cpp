// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024-2026 Thomas Basler and others
 */
#include "RestartHelper.h"
#include "Display_Graphic.h"
#include "Led_Single.h"
#include "RuntimeData.h"
#include <Esp.h>

RestartHelperClass RestartHelper;

RestartHelperClass::RestartHelperClass()
    : _rebootTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&RestartHelperClass::loop, this))
{
}

void RestartHelperClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_rebootTask);
}

void RestartHelperClass::triggerRestart(bool saveRuntimeData)
{
    // sticky: once a caller asked to skip the write, a later request must not re-enable it
    if (!saveRuntimeData) { _saveRuntimeData = false; }

    _rebootTask.enable();
    _rebootTask.restart();
}

void RestartHelperClass::loop()
{
    if (_rebootTask.isFirstIteration()) {
        LedSingle.turnAllOff();
        Display.setStatus(false);

        // write the runtime data to LittleFS, but do not write if last write operation was less than 1 min ago
        if (_saveRuntimeData) { Runtime.writeAll(1); }
    } else {
        ESP.restart();
    }
}

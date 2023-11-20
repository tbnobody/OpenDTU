// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <cstdint>

#define INVERTER_UPDATE_SETTINGS_INTERVAL 60000l

class InverterSettingsClass {
public:
    void init(Scheduler* scheduler);

private:
    void loop();

    Task _loopTask;

    uint32_t _lastUpdate = 0;
};

extern InverterSettingsClass InverterSettings;

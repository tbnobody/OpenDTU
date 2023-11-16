// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

#define INVERTER_UPDATE_SETTINGS_INTERVAL 60000l

class InverterSettingsClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastUpdate = 0;
};

extern InverterSettingsClass InverterSettings;

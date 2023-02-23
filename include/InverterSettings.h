// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

class InverterSettingsClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastUpdate = 0;
};

extern InverterSettingsClass InverterSettings;
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <sunset.h>

#define SUNPOS_UPDATE_INTERVAL 60000l

class SunPositionClass {
public:
    SunPositionClass();
    void init();
    void loop();

    bool isDayPeriod();
    bool isSunsetAvailable();
    bool sunsetTime(struct tm* info);
    bool sunriseTime(struct tm* info);

private:
    void updateSunData();

    SunSet _sun;
    bool _isDayPeriod = true;
    bool _isSunsetAvailable = true;
    uint _sunriseMinutes = 0;
    uint _sunsetMinutes = 0;

    uint32_t _lastUpdate = 0;
    bool _isValidInfo = false;
};

extern SunPositionClass SunPosition;
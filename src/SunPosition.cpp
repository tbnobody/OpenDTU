// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "SunPosition.h"
#include "Configuration.h"
#include "Utils.h"

SunPositionClass SunPosition;

SunPositionClass::SunPositionClass()
{
}

void SunPositionClass::init()
{
}

void SunPositionClass::loop()
{
    if (millis() - _lastUpdate > SUNPOS_UPDATE_INTERVAL) {
        updateSunData();
        _lastUpdate = millis();
    }
}

bool SunPositionClass::isDayPeriod()
{
    return _isDayPeriod;
}

void SunPositionClass::updateSunData()
{
    CONFIG_T const& config = Configuration.get();
    int offset = Utils::getTimezoneOffset() / 3600;
    _sun.setPosition(config.Ntp_Latitude, config.Ntp_Longitude, offset);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        _isDayPeriod = false;
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isValidInfo = false;
        return;
    }

    _sun.setCurrentDate(1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);

    double sunset_type;
    switch (config.Ntp_SunsetType) {
    case 0:
        sunset_type = SunSet::SUNSET_OFFICIAL;
        break;
    case 2:
        sunset_type = SunSet::SUNSET_CIVIL;
        break;
    case 3:
        sunset_type = SunSet::SUNSET_ASTONOMICAL;
        break;
    default:
        sunset_type = SunSet::SUNSET_NAUTICAL;
        break;
    }

    _sunriseMinutes = static_cast<int>(_sun.calcCustomSunrise(sunset_type));
    _sunsetMinutes = static_cast<int>(_sun.calcCustomSunset(sunset_type));
    uint minutesPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    _isDayPeriod = (minutesPastMidnight >= _sunriseMinutes) && (minutesPastMidnight < _sunsetMinutes);
    _isValidInfo = true;
}

bool SunPositionClass::sunsetTime(struct tm* info)
{
    // Get today's date
    time_t aTime = time(NULL);

    // Set the time to midnight
    struct tm tm;
    localtime_r(&aTime, &tm);
    tm.tm_sec = 0;
    tm.tm_min = _sunsetMinutes;
    tm.tm_hour = 0;
    tm.tm_isdst = -1;
    time_t midnight = mktime(&tm);

    localtime_r(&midnight, info);
    return _isValidInfo;
}

bool SunPositionClass::sunriseTime(struct tm* info)
{
    // Get today's date
    time_t aTime = time(NULL);

    // Set the time to midnight
    struct tm tm;
    localtime_r(&aTime, &tm);
    tm.tm_sec = 0;
    tm.tm_min = _sunriseMinutes;
    tm.tm_hour = 0;
    tm.tm_isdst = -1;
    time_t midnight = mktime(&tm);

    localtime_r(&midnight, info);
    return _isValidInfo;
}
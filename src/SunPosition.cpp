// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "SunPosition.h"
#include "Configuration.h"
#include "Utils.h"
#include <Arduino.h>

SunPositionClass SunPosition;

SunPositionClass::SunPositionClass()
{
}

void SunPositionClass::init()
{
}

void SunPositionClass::loop()
{
    if (getDoRecalc() || checkRecalcDayChanged()) {
        updateSunData();
    }
}

bool SunPositionClass::isDayPeriod()
{
    if (!_isValidInfo) {
        return true;
    }

    struct tm timeinfo;
    getLocalTime(&timeinfo, 5);
    uint32_t minutesPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    return (minutesPastMidnight >= _sunriseMinutes) && (minutesPastMidnight < _sunsetMinutes);
}

bool SunPositionClass::isSunsetAvailable()
{
    return _isSunsetAvailable;
}

void SunPositionClass::setDoRecalc(bool doRecalc)
{
    std::lock_guard<std::mutex> lock(_recalcLock);
    _doRecalc = doRecalc;
}

bool SunPositionClass::getDoRecalc()
{
    std::lock_guard<std::mutex> lock(_recalcLock);
    return _doRecalc;
}

bool SunPositionClass::checkRecalcDayChanged()
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo); // don't use getLocalTime() as there could be a delay of 10ms

    uint32_t ymd;
    ymd = (timeinfo.tm_year << 9) | (timeinfo.tm_mon << 5) | timeinfo.tm_mday;

    if (_lastSunPositionCalculatedYMD != ymd) {
        return true;
    }
    return false;
}

void SunPositionClass::updateSunData()
{
    struct tm timeinfo;
    bool gotLocalTime;

    gotLocalTime = getLocalTime(&timeinfo, 5);
    _lastSunPositionCalculatedYMD = (timeinfo.tm_year << 9) | (timeinfo.tm_mon << 5) | timeinfo.tm_mday;
    setDoRecalc(false);

    if (!gotLocalTime) {
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isValidInfo = false;
        return;
    }

    CONFIG_T const& config = Configuration.get();
    int offset = Utils::getTimezoneOffset() / 3600;

    _sun.setPosition(config.Ntp_Latitude, config.Ntp_Longitude, offset);
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

    double sunriseRaw = _sun.calcCustomSunrise(sunset_type);
    double sunsetRaw = _sun.calcCustomSunset(sunset_type);

    // If no sunset/sunrise exists (e.g. astronomical calculation in summer)
    // assume it's day period
    if (std::isnan(sunriseRaw) || std::isnan(sunsetRaw)) {
        _isSunsetAvailable = false;
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isValidInfo = false;
        return;
    }

    _sunriseMinutes = static_cast<int>(sunriseRaw);
    _sunsetMinutes = static_cast<int>(sunsetRaw);

    _isSunsetAvailable = true;
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

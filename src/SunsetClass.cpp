#include "SunsetClass.h"
#include "Configuration.h"
#include "defaults.h"
#include <cstdlib>
#include <time.h>

extern long timezone;

SunsetClass::SunsetClass()
{
    _initialized = false;
}

void SunsetClass::init()
{
    setLocation();
    _isDayTime = true;
}

void SunsetClass::loop()
{
    if (!Configuration.get().Sunset_Enabled) {
        if (_initialized) {
            _isDayTime = true;
            _currentDay = -1;
            _sunriseMinutes = _sunsetMinutes = 0;
            _initialized = false;
        }
        return;
    }
    if(!_initialized)
        setLocation();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 0)) { // Time is not valid
        _isDayTime = true;
        _currentDay = -1;
        _sunriseMinutes = _sunsetMinutes = 0;
        return;
    }

    if (_currentDay != timeinfo.tm_mday) {
        _currentDay = timeinfo.tm_mday;
        _sunSet.setCurrentDate(1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);

        // If you have daylight savings time, make sure you set the timezone appropriately as well
        _sunSet.setTZOffset(_timezoneOffset + (timeinfo.tm_isdst != 0 ? 1 : 0));
        _sunriseMinutes = (int)_sunSet.calcSunrise();
        _sunsetMinutes = (int)_sunSet.calcSunset();

        int secondsPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        _isDayTime = (secondsPastMidnight >= (_sunriseMinutes + Configuration.get().Sunset_Sunriseoffset)) 
            && (secondsPastMidnight < (_sunsetMinutes + Configuration.get().Sunset_Sunsetoffset));
    }
}

void SunsetClass::setLocation()
{
    _latitude = std::atoi(Configuration.get().Sunset_Latitude);
    _longitude = std::atoi(Configuration.get().Sunset_Longitude);

    // Set default values
    _currentDay = -1;
    _isDayTime = true;
    _sunriseMinutes = _sunsetMinutes = 0;

    // Get timezone offset
    struct tm dt;
    memset(&dt, 0, sizeof(struct tm));
    dt.tm_mday = 1;
    dt.tm_year = 70;
    time_t tzlag = mktime(&dt);
    _timezoneOffset = -tzlag / 3600;

    _sunSet.setPosition(_latitude, _longitude, (double)_timezoneOffset);

    _initialized = true;
}

int SunsetClass::getTimezoneOffset()
{
    return _timezoneOffset;
}

int SunsetClass::getSunriseMinutes()
{
    return _sunriseMinutes;
}

int SunsetClass::getSunsetMinutes()
{
    return _sunsetMinutes;
}

bool SunsetClass::isDayTime()
{
    return _isDayTime;
}

SunsetClass SunsetClassInst;
#pragma once

#include <Arduino.h>
#include <sunset.h>

class SunsetClass {
public:
    SunsetClass();
    void init();
    void loop();

    void setLocation();
    int getTimezoneOffset();
    int getSunriseMinutes();
    int getSunsetMinutes();
    bool isDayTime();

private:
    SunSet _sunSet;
    bool _initialized;
    double _latitude;
    double _longitude;
    int _currentDay;
    int _timezoneOffset;
    bool _isDayTime;
    int _sunriseMinutes;
    int _sunsetMinutes;
};

extern SunsetClass SunsetClassInst;

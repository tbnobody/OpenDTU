#include "TimeoutHelper.h"
#include <Arduino.h>

TimeoutHelper::TimeoutHelper()
{
    timeout = 0;
    startMillis = 0;
}

void TimeoutHelper::set(uint32_t ms)
{
    timeout = ms;
    startMillis = millis();
}

void TimeoutHelper::extend(uint32_t ms)
{
    timeout += ms;
}

bool TimeoutHelper::occured()
{
    return millis() > (startMillis + timeout);
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
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

void TimeoutHelper::reset()
{
    startMillis = millis();
}

bool TimeoutHelper::occured()
{
    return millis() > (startMillis + timeout);
}
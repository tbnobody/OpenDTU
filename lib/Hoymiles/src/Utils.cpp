// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2025 Thomas Basler and others
 */
#include "Utils.h"
#include <Arduino.h>
#include <time.h>

uint8_t Utils::getWeekDay()
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    return tm.tm_mday;
}

bool Utils::getTimeAvailable()
{
    struct tm timeinfo;
    return getLocalTime(&timeinfo, 5);
}

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

String Utils::dumpArray(const uint8_t data[], const uint8_t len)
{
    if (len == 0) {
        return String();
    }

    // Each byte needs 2 hex chars + 1 space (except last byte)
    String result;
    result.reserve(len * 3);

    char buf[4]; // Buffer for single hex value + space + null
    for (uint8_t i = 0; i < len; i++) {
        snprintf(buf, sizeof(buf), "%02X%s", data[i], (i < len - 1) ? " " : "");
        result += buf;
    }

    return result;
}

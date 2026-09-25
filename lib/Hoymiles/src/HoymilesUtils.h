// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <WString.h>
#include <cstdint>

class HoymilesUtils {
public:
    static uint8_t getWeekDay();
    static bool getTimeAvailable();
    static String dumpArray(const uint8_t buf[], const uint8_t len);
    static String formatSerial(uint64_t serial);
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

class Utils {
public:
    static uint32_t getChipId();
    static uint64_t generateDtuSerial();
    static int getTimezoneOffset();
};

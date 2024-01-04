// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <cstdint>

class Utils {
public:
    static uint32_t getChipId();
    static uint64_t generateDtuSerial();
    static int getTimezoneOffset();
    static void restartDtu();
    static bool checkJsonAlloc(const DynamicJsonDocument& doc, const char* function, const uint16_t line);
};

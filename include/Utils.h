// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <cstdint>

class Utils {
public:
    static uint32_t getChipId();
    static uint64_t generateDtuSerial();
    static int getTimezoneOffset();
    static bool checkJsonAlloc(const JsonDocument& doc, const char* function, const uint16_t line);
    static void removeAllFiles();
    static String generateMd5FromFile(String file);
    static void skipBom(File& f);
};

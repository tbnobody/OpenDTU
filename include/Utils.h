// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <cstdint>
#include <utility>

class Utils {
public:
    static uint32_t getChipId();
    static uint64_t generateDtuSerial();
    static int getTimezoneOffset();
    static void restartDtu();
    static bool checkJsonAlloc(const JsonDocument& doc, const char* function, const uint16_t line);
    static void removeAllFiles();

    /* OpenDTU-OnBatter-specific utils go here: */
    template<typename T>
    static std::pair<T, String> getJsonValueByPath(JsonDocument const& root, String const& path);

    template <typename T>
    static std::optional<T> getNumericValueFromMqttPayload(char const* client,
            std::string const& src, char const* topic, char const* jsonPath);
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <cstdint>
#include <utility>

class Utils {
public:
    static uint32_t getChipId();
    static uint64_t generateDtuSerial();
    static int getTimezoneOffset();
    static bool checkJsonAlloc(const JsonDocument& doc, const char* function, const uint16_t line);
    static void removeAllFiles();
    static String generateMd5FromFile(String file);
    static void skipBom(File& f);

    /* OpenDTU-OnBatter-specific utils go here: */
    template<typename T>
    static std::optional<T> getFromString(char const* val);

    template<typename T>
    static std::pair<T, String> getJsonValueByPath(JsonDocument const& root, String const& path);

    template <typename T>
    static std::optional<T> getNumericValueFromMqttPayload(char const* client,
            std::string const& src, char const* topic, char const* jsonPath);

    template<typename T>
    static std::optional<T> getJsonElement(JsonObjectConst const root, char const* key, size_t nesting = 0) {
        if (!root[key].isNull() && root[key].is<T>() && root[key].nesting() == nesting) {
            return root[key].as<T>();
        }
        return std::nullopt;
    }

    static bool getEpoch(time_t* epoch, uint32_t ms = 20);
};

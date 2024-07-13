// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2024 Thomas Basler and others
 */

#include "Utils.h"
#include "Display_Graphic.h"
#include "Led_Single.h"
#include "MessageOutput.h"
#include <Esp.h>
#include <LittleFS.h>

uint32_t Utils::getChipId()
{
    uint32_t chipId = 0;
    for (uint8_t i = 0; i < 17; i += 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return chipId;
}

uint64_t Utils::generateDtuSerial()
{
    uint32_t chipId = getChipId();
    uint64_t dtuId = 0;

    // Product category (char 1-4): 1 = Micro Inverter, 999 = Dummy
    dtuId |= 0x199900000000;

    // Year of production (char 5): 1 equals 2015 so hard code 8 = 2022
    dtuId |= 0x80000000;

    // Week of production (char 6-7): Range is 1-52 s hard code 1 = week 1
    dtuId |= 0x0100000;

    // Running Number (char 8-12): Derived from the ESP chip id
    for (uint8_t i = 0; i < 5; i++) {
        dtuId |= (chipId % 10) << (i * 4);
        chipId /= 10;
    }

    return dtuId;
}

int Utils::getTimezoneOffset()
{
    // see: https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c/44063597#44063597

    time_t gmt, rawtime = time(NULL);
    struct tm* ptm;

    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);

    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return static_cast<int>(difftime(rawtime, gmt));
}

void Utils::restartDtu()
{
    LedSingle.turnAllOff();
    Display.setStatus(false);
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

bool Utils::checkJsonAlloc(const JsonDocument& doc, const char* function, const uint16_t line)
{
    if (doc.overflowed()) {
        MessageOutput.printf("Alloc failed: %s, %d\r\n", function, line);
        return false;
    }

    return true;
}

/// @brief Remove all files but the PINMAPPING_FILENAME
void Utils::removeAllFiles()
{
    auto root = LittleFS.open("/");
    auto file = root.getNextFileName();

    while (file != "") {
        if (file != PINMAPPING_FILENAME) {
            LittleFS.remove(file);
        }
        file = root.getNextFileName();
    }
}

/* OpenDTU-OnBatter-specific utils go here: */
template<typename T>
std::optional<T> getFromString(char const* val);

template<>
std::optional<float> getFromString(char const* val)
{
    float res = 0;

    try {
        res = std::stof(val);
    }
    catch (std::invalid_argument const& e) {
        return std::nullopt;
    }

    return res;
}

template<typename T>
char const* getTypename();

template<>
char const* getTypename<float>() { return "float"; }

template<typename T>
std::pair<T, String> Utils::getJsonValueByPath(JsonDocument const& root, String const& path)
{
    size_t constexpr kErrBufferSize = 256;
    char errBuffer[kErrBufferSize];
    constexpr char delimiter = '/';
    int start = 0;
    int end = path.indexOf(delimiter);
    auto value = root.as<JsonVariantConst>();

    // NOTE: "Because ArduinoJson implements the Null Object Pattern, it is
    // always safe to read the object: if the key doesn't exist, it returns an
    // empty value."
    auto getNext = [&](String const& key) -> bool {
        // handle double forward slashes and paths starting or ending with a slash
        if (key.isEmpty()) { return true; }

        if (key[0] == '[' && key[key.length() - 1] == ']') {
            if (!value.is<JsonArrayConst>()) {
                snprintf(errBuffer, kErrBufferSize, "Cannot access non-array "
                        "JSON node using array index '%s' (JSON path '%s', "
                        "position %i)", key.c_str(), path.c_str(), start);
                return false;
            }

            auto idx = key.substring(1, key.length() - 1).toInt();
            value = value[idx];

            if (value.isNull()) {
                snprintf(errBuffer, kErrBufferSize, "Unable to access JSON "
                        "array index %li (JSON path '%s', position %i)",
                        idx, path.c_str(), start);
                return false;
            }

            return true;
        }

        value = value[key];

        if (value.isNull()) {
            snprintf(errBuffer, kErrBufferSize, "Unable to access JSON key "
                    "'%s' (JSON path '%s', position %i)",
                    key.c_str(), path.c_str(), start);
            return false;
        }

        return true;
    };

    while (end != -1) {
        if (!getNext(path.substring(start, end))) {
              return { T(), String(errBuffer) };
        }
        start = end + 1;
        end = path.indexOf(delimiter, start);
    }

    if (!getNext(path.substring(start))) {
        return { T(), String(errBuffer) };
    }

    if (value.is<T>()) {
        return { value.as<T>(), "" };
    }

    if (!value.is<char const*>()) {
        snprintf(errBuffer, kErrBufferSize, "Value '%s' at JSON path '%s' is "
                "neither a string nor of type %s", value.as<String>().c_str(),
                path.c_str(), getTypename<T>());
        return { T(), String(errBuffer) };
    }

    auto res = getFromString<T>(value.as<char const*>());
    if (!res.has_value()) {
        snprintf(errBuffer, kErrBufferSize, "String '%s' at JSON path '%s' cannot "
                "be converted to %s", value.as<String>().c_str(), path.c_str(),
                getTypename<T>());
        return { T(), String(errBuffer) };
    }

    return { *res, "" };
}

template std::pair<float, String> Utils::getJsonValueByPath(JsonDocument const& root, String const& path);

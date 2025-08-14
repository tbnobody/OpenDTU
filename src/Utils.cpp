// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */

#include "Utils.h"
#include "PinMapping.h"
#include <LittleFS.h>
#include <MD5Builder.h>

#undef TAG
static const char* TAG = "utils";

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

bool Utils::checkJsonAlloc(const JsonDocument& doc, const char* function, const uint16_t line)
{
    if (doc.overflowed()) {
        ESP_LOGE(TAG, "Alloc failed: %s, %" PRIu16 "", function, line);
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

String Utils::generateMd5FromFile(String file)
{
    if (!LittleFS.exists(file)) {
        return String();
    }

    File f = LittleFS.open(file, "r");
    if (!file) {
        return String();
    }

    MD5Builder md5;
    md5.begin();

    // Read the file in chunks to avoid using too much memory
    uint8_t buffer[512];

    while (f.available()) {
        size_t bytesRead = f.read(buffer, sizeof(buffer) / sizeof(buffer[0]));
        md5.add(buffer, bytesRead);
    }

    // Finalize and calculate the MD5 hash
    md5.calculate();

    f.close();

    return md5.toString();
}

void Utils::skipBom(File& f)
{
    // skip Byte Order Mask (BOM). valid JSON docs always start with '{' or '['.
    while (f.available() > 0) {
        int c = f.peek();
        if (c == '{' || c == '[') {
            break;
        }
        f.read();
    }
}

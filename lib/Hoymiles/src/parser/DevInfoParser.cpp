// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2024 Thomas Basler and others
 */

/*
This parser is used to parse the response of 'DevInfoAllCommand' and 'DevInfoSimpleCommand'.
It contains version information of the hardware and firmware. It can also be used to determine
the exact inverter type.

Data structure (DevInfoAllCommand):

00   01 02 03 04   05 06 07 08   09   10 11       12 13    14 15          16 17           18 19       20 21   22 23   24 25   26   27 28 29 30 31
                                      00 01       02 03    04 05          06 07           08 09       10 11   12 13
-------------------------------------------------------------------------------------------------------------------------------------------------
95   80 14 82 66   80 14 33 28   81   27 1C       07 E5    04 01          07 2D           00 01       00 00   00 00   DF DD   1E   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^       ^^^^^    ^^^^^          ^^^^^           ^^^^^       ^^^^^   ^^^^^   ^^^^^   ^^
ID   Source Addr   Target Addr   Idx  FW Version  FW Year  FW Month/Date  FW Hour/Minute  Bootloader  ?       ?       CRC16   CRC8


Data structure (DevInfoSimpleCommand):

00   01 02 03 04   05 06 07 08   09   10 11       12 13 14 15   16 17        18 19   20 21   22 23   24 25   26   27 28 29 30 31
                                      00 01       02 03 04 05   06 07        08 09   10 11   12 13
-------------------------------------------------------------------------------------------------------------------------------------------------
95   80 14 82 66   80 14 33 28   81   27 1C       10 12 71 01   01 00        0A 00   20 01   00 00   E5 F8   95
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^       ^^^^^^^^^^^   ^^^^^        ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^
ID   Source Addr   Target Addr   Idx  FW Version  HW Part No.   HW Version   ?       ?       ?       CRC16   CRC8
*/
#include "DevInfoParser.h"
#include "../Hoymiles.h"
#include <cstring>

#define ALL 0xff

typedef struct {
    uint8_t hwPart[4];
    uint16_t maxPower;
    const char* modelName;
} devInfo_t;

const devInfo_t devInfo[] = {
    { { 0x10, 0x10, 0x10, ALL }, 300, "HM-300-1T" },
    { { 0x10, 0x10, 0x20, ALL }, 350, "HM-350-1T" },
    { { 0x10, 0x10, 0x30, ALL }, 400, "HM-400-1T" },
    { { 0x10, 0x10, 0x40, ALL }, 400, "HM-400-1T" },
    { { 0x10, 0x11, 0x10, ALL }, 600, "HM-600-2T" },
    { { 0x10, 0x11, 0x20, ALL }, 700, "HM-700-2T" },
    { { 0x10, 0x11, 0x30, ALL }, 800, "HM-800-2T" },
    { { 0x10, 0x11, 0x40, ALL }, 800, "HM-800-2T" },
    { { 0x10, 0x12, 0x10, ALL }, 1200, "HM-1200-4T" },
    { { 0x10, 0x02, 0x30, ALL }, 1500, "MI-1500-4T Gen3" },
    { { 0x10, 0x12, 0x30, ALL }, 1500, "HM-1500-4T" },
    { { 0x10, 0x10, 0x10, 0x15 }, static_cast<uint16_t>(300 * 0.7), "HM-300-1T" }, // HM-300 factory limitted to 70%

    { { 0x10, 0x20, 0x11, ALL }, 300, "HMS-300-1T" }, // 00
    { { 0x10, 0x20, 0x21, ALL }, 350, "HMS-350-1T" }, // 00
    { { 0x10, 0x20, 0x41, ALL }, 400, "HMS-400-1T" }, // 00
    { { 0x10, 0x10, 0x51, ALL }, 450, "HMS-450-1T" }, // 01
    { { 0x10, 0x20, 0x51, ALL }, 450, "HMS-450-1T" }, // 03
    { { 0x10, 0x10, 0x71, ALL }, 500, "HMS-500-1T" }, // 02
    { { 0x10, 0x20, 0x71, ALL }, 500, "HMS-500-1T v2" }, // 02
    { { 0x10, 0x21, 0x11, ALL }, 600, "HMS-600-2T" }, // 01
    { { 0x10, 0x21, 0x21, ALL }, 700, "HMS-700-2T" }, // 00
    { { 0x10, 0x21, 0x41, ALL }, 800, "HMS-800-2T" }, // 00
    { { 0x10, 0x11, 0x41, ALL }, 800, "HMS-800-2T-LV" }, // 00
    { { 0x10, 0x11, 0x51, ALL }, 900, "HMS-900-2T" }, // 01
    { { 0x10, 0x21, 0x51, ALL }, 900, "HMS-900-2T" }, // 03
    { { 0x10, 0x21, 0x71, ALL }, 1000, "HMS-1000-2T" }, // 05
    { { 0x10, 0x11, 0x71, ALL }, 1000, "HMS-1000-2T" }, // 01
    { { 0x10, 0x22, 0x41, ALL }, 1600, "HMS-1600-4T" }, // 4
    { { 0x10, 0x12, 0x51, ALL }, 1800, "HMS-1800-4T" }, // 01
    { { 0x10, 0x22, 0x51, ALL }, 1800, "HMS-1800-4T" }, // 16
    { { 0x10, 0x12, 0x71, ALL }, 2000, "HMS-2000-4T" }, // 01
    { { 0x10, 0x22, 0x71, ALL }, 2000, "HMS-2000-4T" }, // 10

    { { 0x10, 0x32, 0x41, ALL }, 1600, "HMT-1600-4T" }, // 00
    { { 0x10, 0x32, 0x51, ALL }, 1800, "HMT-1800-4T" }, // 00
    { { 0x10, 0x32, 0x71, ALL }, 2000, "HMT-2000-4T" }, // 0

    { { 0x10, 0x33, 0x11, ALL }, 1800, "HMT-1800-6T" }, // 01
    { { 0x10, 0x33, 0x31, ALL }, 2250, "HMT-2250-6T" }, // 01

    { { 0xF1, 0x01, 0x10, ALL }, 600, "HERF-600" }, // 00
    { { 0xF1, 0x01, 0x14, ALL }, 800, "HERF-800" }, // 00
    { { 0xF1, 0x01, 0x24, ALL }, 1600, "HERF-1600" }, // 00
    { { 0xF1, 0x01, 0x22, ALL }, 1800, "HERF-1800" }, // 00
};

DevInfoParser::DevInfoParser()
    : Parser()
{
    clearBufferSimple();
    clearBufferAll();
}

void DevInfoParser::clearBufferAll()
{
    memset(_payloadDevInfoAll, 0, DEV_INFO_SIZE);
    _devInfoAllLength = 0;
}

void DevInfoParser::appendFragmentAll(const uint8_t offset, const uint8_t* payload, const uint8_t len)
{
    if (offset + len > DEV_INFO_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) dev info all packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadDevInfoAll[offset], payload, len);
    _devInfoAllLength += len;
}

void DevInfoParser::clearBufferSimple()
{
    memset(_payloadDevInfoSimple, 0, DEV_INFO_SIZE);
    _devInfoSimpleLength = 0;
}

void DevInfoParser::appendFragmentSimple(const uint8_t offset, const uint8_t* payload, const uint8_t len)
{
    if (offset + len > DEV_INFO_SIZE) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) dev info Simple packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadDevInfoSimple[offset], payload, len);
    _devInfoSimpleLength += len;
}

uint32_t DevInfoParser::getLastUpdateAll() const
{
    return _lastUpdateAll;
}

void DevInfoParser::setLastUpdateAll(const uint32_t lastUpdate)
{
    _lastUpdateAll = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint32_t DevInfoParser::getLastUpdateSimple() const
{
    return _lastUpdateSimple;
}

void DevInfoParser::setLastUpdateSimple(const uint32_t lastUpdate)
{
    _lastUpdateSimple = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint16_t DevInfoParser::getFwBuildVersion() const
{
    HOY_SEMAPHORE_TAKE();
    const uint16_t ret = (static_cast<uint16_t>(_payloadDevInfoAll[0]) << 8) | _payloadDevInfoAll[1];
    HOY_SEMAPHORE_GIVE();
    return ret;
}

time_t DevInfoParser::getFwBuildDateTime() const
{
    struct tm timeinfo = {};
    HOY_SEMAPHORE_TAKE();
    timeinfo.tm_year = ((static_cast<uint16_t>(_payloadDevInfoAll[2]) << 8) | _payloadDevInfoAll[3]) - 1900;

    timeinfo.tm_mon = ((static_cast<uint16_t>(_payloadDevInfoAll[4]) << 8) | _payloadDevInfoAll[5]) / 100 - 1;
    timeinfo.tm_mday = ((static_cast<uint16_t>(_payloadDevInfoAll[4]) << 8) | _payloadDevInfoAll[5]) % 100;

    timeinfo.tm_hour = ((static_cast<uint16_t>(_payloadDevInfoAll[6]) << 8) | _payloadDevInfoAll[7]) / 100;
    timeinfo.tm_min = ((static_cast<uint16_t>(_payloadDevInfoAll[6]) << 8) | _payloadDevInfoAll[7]) % 100;
    HOY_SEMAPHORE_GIVE();

    return timegm(&timeinfo);
}

String DevInfoParser::getFwBuildDateTimeStr() const
{
    char timebuffer[32];
    const time_t t = getFwBuildDateTime();
    std::strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
    return timebuffer;
}

uint16_t DevInfoParser::getFwBootloaderVersion() const
{
    HOY_SEMAPHORE_TAKE();
    const uint16_t ret = (static_cast<uint16_t>(_payloadDevInfoAll[8]) << 8) | _payloadDevInfoAll[9];
    HOY_SEMAPHORE_GIVE();
    return ret;
}

uint32_t DevInfoParser::getHwPartNumber() const
{
    HOY_SEMAPHORE_TAKE();
    const uint16_t hwpn_h = (static_cast<uint16_t>(_payloadDevInfoSimple[2]) << 8) | _payloadDevInfoSimple[3];
    const uint16_t hwpn_l = (static_cast<uint16_t>(_payloadDevInfoSimple[4]) << 8) | _payloadDevInfoSimple[5];
    HOY_SEMAPHORE_GIVE();

    return (static_cast<uint32_t>(hwpn_h) << 16) | static_cast<uint32_t>(hwpn_l);
}

String DevInfoParser::getHwVersion() const
{
    char buf[8];
    HOY_SEMAPHORE_TAKE();
    snprintf(buf, sizeof(buf), "%02d.%02d", _payloadDevInfoSimple[6], _payloadDevInfoSimple[7]);
    HOY_SEMAPHORE_GIVE();
    return buf;
}

uint16_t DevInfoParser::getMaxPower() const
{
    const uint8_t idx = getDevIdx();
    if (idx == 0xff) {
        return 0;
    }
    return devInfo[idx].maxPower;
}

String DevInfoParser::getHwModelName() const
{
    const uint8_t idx = getDevIdx();
    if (idx == 0xff) {
        return "";
    }
    return devInfo[idx].modelName;
}

bool DevInfoParser::containsValidData() const
{
    const time_t t = getFwBuildDateTime();

    struct tm info;
    localtime_r(&t, &info);

    return info.tm_year > (2016 - 1900) && getHwPartNumber() != 124097;
}

uint8_t DevInfoParser::getDevIdx() const
{
    uint8_t ret = 0xff;
    uint8_t pos;

    HOY_SEMAPHORE_TAKE();

    // Check for all 4 bytes first
    for (pos = 0; pos < sizeof(devInfo) / sizeof(devInfo_t); pos++) {
        if (devInfo[pos].hwPart[0] == _payloadDevInfoSimple[2]
            && devInfo[pos].hwPart[1] == _payloadDevInfoSimple[3]
            && devInfo[pos].hwPart[2] == _payloadDevInfoSimple[4]
            && devInfo[pos].hwPart[3] == _payloadDevInfoSimple[5]) {
            ret = pos;
            break;
        }
    }

    // Then only for 3 bytes but only if not already found
    if (ret == 0xff) {
        for (pos = 0; pos < sizeof(devInfo) / sizeof(devInfo_t); pos++) {
            if (devInfo[pos].hwPart[0] == _payloadDevInfoSimple[2]
                && devInfo[pos].hwPart[1] == _payloadDevInfoSimple[3]
                && devInfo[pos].hwPart[2] == _payloadDevInfoSimple[4]) {
                ret = pos;
                break;
            }
        }
    }

    HOY_SEMAPHORE_GIVE();

    return ret;
}

/* struct tm to seconds since Unix epoch */
time_t DevInfoParser::timegm(const struct tm* t)
{
    uint32_t year;
    time_t result;
#define MONTHSPERYEAR 12 /* months per calendar year */
    static const int cumdays[MONTHSPERYEAR] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    /*@ +matchanyintegral @*/
    year = 1900 + t->tm_year + t->tm_mon / MONTHSPERYEAR;
    result = (year - 1970) * 365 + cumdays[t->tm_mon % MONTHSPERYEAR];
    result += (year - 1968) / 4;
    result -= (year - 1900) / 100;
    result += (year - 1600) / 400;
    if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) && (t->tm_mon % MONTHSPERYEAR) < 2)
        result--;
    result += t->tm_mday - 1;
    result *= 24;
    result += t->tm_hour;
    result *= 60;
    result += t->tm_min;
    result *= 60;
    result += t->tm_sec;
    if (t->tm_isdst == 1)
        result -= 3600;
    /*@ -matchanyintegral @*/
    return (result);
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */

/*
This parser is used to parse the response of 'RfInfoCommand'.
It contains version information of the inverter's built-in RF module.

Data structure:

00 01   02 03 04 05   06 07 08 09    10 11 12 13
-------------------------------------------------
PID     Serial        RFHW Version   RFFW Version
*/
#include "RfInfoParser.h"
#include <array>
#include <cstdio>
#include <esp_log.h>

#undef TAG
static const char* TAG = "hoymiles";

bool RfInfoParser::setPayload(const uint8_t* payload, const uint8_t len)
{
    if (len < RF_INFO_SIZE) {
        ESP_LOGE(TAG, "(%s, %d) rf info packet too short (%u)", __FILE__, __LINE__, static_cast<unsigned>(len));
        return false;
    }

    const uint32_t rfSw = (static_cast<uint32_t>(payload[RF_FW_VERSION_OFFSET]) << 24)
        | (static_cast<uint32_t>(payload[RF_FW_VERSION_OFFSET + 1]) << 16)
        | (static_cast<uint32_t>(payload[RF_FW_VERSION_OFFSET + 2]) << 8)
        | static_cast<uint32_t>(payload[RF_FW_VERSION_OFFSET + 3]);
    const RfModuleLine line = lineFromVersion(rfSw);
    const bool hasStatus = (len > RF_STATUS_OFFSET) && (line != RfModuleLine::A);

    HOY_SEMAPHORE_TAKE();
    _rfHwVersion = versionStr(payload, RF_HW_VERSION_OFFSET);
    _rfFwVersion = versionStr(payload, RF_FW_VERSION_OFFSET);
    _rfFwRaw = rfSw;
    _line = line;
    _hasStatus = hasStatus;
    _status = hasStatus ? payload[RF_STATUS_OFFSET] : 0;
    HOY_SEMAPHORE_GIVE();

    ESP_LOGI(TAG, "RF module: HW %s, FW %s, line %c", _rfHwVersion.c_str(), _rfFwVersion.c_str(),
        getModuleLineChar());

    return true;
}

RfModuleLine RfInfoParser::lineFromVersion(const uint32_t rfSw)
{
    const uint8_t major = rfSw >> 24;
    const uint8_t minor = (rfSw >> 16) & 0xff;

    if (major == 0x00 && rfSw != 0) {
        return RfModuleLine::A;
    }

    if (major == 0x02) {
        if (minor >= 0x07) {
            return RfModuleLine::D;
        }
        if (minor >= 0x05) {
            return RfModuleLine::C;
        }
        if (minor >= 0x03) {
            return RfModuleLine::B;
        }
    }

    return RfModuleLine::Unknown;
}

uint32_t RfInfoParser::getRfFirmwareVersionRaw() const
{
    HOY_SEMAPHORE_TAKE();
    const uint32_t v = _rfFwRaw;
    HOY_SEMAPHORE_GIVE();
    return v;
}

RfModuleLine RfInfoParser::getModuleLine() const
{
    HOY_SEMAPHORE_TAKE();
    const RfModuleLine v = _line;
    HOY_SEMAPHORE_GIVE();
    return v;
}

char RfInfoParser::getModuleLineChar() const
{
    switch (getModuleLine()) {
    case RfModuleLine::A:
        return 'A';
    case RfModuleLine::B:
        return 'B';
    case RfModuleLine::C:
        return 'C';
    case RfModuleLine::D:
        return 'D';
    default:
        return '?';
    }
}

bool RfInfoParser::hasModuleStatus() const
{
    HOY_SEMAPHORE_TAKE();
    const bool v = _hasStatus;
    HOY_SEMAPHORE_GIVE();
    return v;
}

uint8_t RfInfoParser::getModuleStatus() const
{
    HOY_SEMAPHORE_TAKE();
    const uint8_t v = _status;
    HOY_SEMAPHORE_GIVE();
    return v;
}

String RfInfoParser::versionStr(const uint8_t* payload, const size_t offset)
{
    std::array<char, 16> buf = {};
    snprintf(buf.data(), buf.size(), "%02x.%02x.%02x.%02x",
        payload[offset], payload[offset + 1], payload[offset + 2], payload[offset + 3]);
    return buf.data();
}

String RfInfoParser::getRfHardwareVersionStr() const
{
    HOY_SEMAPHORE_TAKE();
    String v = _rfHwVersion;
    HOY_SEMAPHORE_GIVE();
    return v;
}

String RfInfoParser::getRfFirmwareVersionStr() const
{
    HOY_SEMAPHORE_TAKE();
    String v = _rfFwVersion;
    HOY_SEMAPHORE_GIVE();
    return v;
}

bool RfInfoParser::containsValidData() const
{
    return getLastUpdate() > 0;
}

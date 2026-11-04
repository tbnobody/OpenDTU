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

    HOY_SEMAPHORE_TAKE();
    _rfHwVersion = versionStr(payload, RF_HW_VERSION_OFFSET);
    _rfFwVersion = versionStr(payload, RF_FW_VERSION_OFFSET);
    HOY_SEMAPHORE_GIVE();

    return true;
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

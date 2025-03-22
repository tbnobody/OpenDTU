// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2024 Thomas Basler and others
 */

/*
This parser is used to parse the response of 'SystemConfigParaCommand'.
It contains the set inverter limit.

Data structure:

00   01 02 03 04   05 06 07 08   09   10 11   12 13           14 15   16 17   18 19   20 21   22 23   24 25   26   27 28 29 30 31
                                      00 01   02 03           04 05   06 07   08 09   10 11   12 13
---------------------------------------------------------------------------------------------------------------------------------
95   80 14 82 66   80 14 33 28   81   00 01   03 E8           00 00   03 E8   00 00   00 00   00 00   3C F8   2E   -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^   ^^^^^           ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^^^^   ^^
ID   Source Addr   Target Addr   Idx  ?       Limit percent   ?       ?       ?       ?       ?       CRC16   CRC8
*/
#include "SystemConfigParaParser.h"
#include "../Hoymiles.h"
#include <cstring>

SystemConfigParaParser::SystemConfigParaParser()
    : Parser()
{
    clearBuffer();
}

void SystemConfigParaParser::clearBuffer()
{
    memset(_payload, 0, SYSTEM_CONFIG_PARA_SIZE);
    _payloadLength = 0;
}

void SystemConfigParaParser::appendFragment(const uint8_t offset, const uint8_t* payload, const uint8_t len)
{
    if (offset + len > (SYSTEM_CONFIG_PARA_SIZE)) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) stats packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payload[offset], payload, len);
    _payloadLength += len;
}

float SystemConfigParaParser::getLimitPercent() const
{
    HOY_SEMAPHORE_TAKE();
    const float ret = ((static_cast<uint16_t>(_payload[2]) << 8) | _payload[3]) / 10.0;
    HOY_SEMAPHORE_GIVE();

    // don't pretend the inverter could produce more than its rated power,
    // even though it does process, accept, and even save limit values beyond
    // its rated power.
    return min<float>(100, ret);
}

void SystemConfigParaParser::setLimitPercent(const float value)
{
    const uint16_t val = static_cast<uint16_t>(value * 10);
    HOY_SEMAPHORE_TAKE();
    _payload[2] = val >> 8;
    _payload[3] = val & 0xFF;
    HOY_SEMAPHORE_GIVE();
}

void SystemConfigParaParser::setLastLimitCommandSuccess(const LastCommandSuccess status)
{
    _lastLimitCommandSuccess = status;
}

LastCommandSuccess SystemConfigParaParser::getLastLimitCommandSuccess() const
{
    return _lastLimitCommandSuccess;
}

uint32_t SystemConfigParaParser::getLastUpdateCommand() const
{
    return _lastUpdateCommand;
}

void SystemConfigParaParser::setLastUpdateCommand(const uint32_t lastUpdate)
{
    _lastUpdateCommand = lastUpdate;
    setLastUpdate(lastUpdate);
}

void SystemConfigParaParser::setLastLimitRequestSuccess(const LastCommandSuccess status)
{
    _lastLimitRequestSuccess = status;
}

LastCommandSuccess SystemConfigParaParser::getLastLimitRequestSuccess() const
{
    return _lastLimitRequestSuccess;
}

uint32_t SystemConfigParaParser::getLastUpdateRequest() const
{
    return _lastUpdateRequest;
}

void SystemConfigParaParser::setLastUpdateRequest(const uint32_t lastUpdate)
{
    _lastUpdateRequest = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint8_t SystemConfigParaParser::getExpectedByteCount() const
{
    return SYSTEM_CONFIG_PARA_SIZE;
}

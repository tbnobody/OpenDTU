// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
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
    const float ret = ((((uint16_t)_payload[2]) << 8) | _payload[3]) / 10.0;
    HOY_SEMAPHORE_GIVE();
    return ret;
}

void SystemConfigParaParser::setLimitPercent(const float value)
{
    HOY_SEMAPHORE_TAKE();
    _payload[2] = ((uint16_t)(value * 10)) >> 8;
    _payload[3] = ((uint16_t)(value * 10));
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

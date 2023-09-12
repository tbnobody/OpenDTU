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

void SystemConfigParaParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > (SYSTEM_CONFIG_PARA_SIZE)) {
        Hoymiles.getMessageOutput()->printf("FATAL: (%s, %d) stats packet too large for buffer\r\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payload[offset], payload, len);
    _payloadLength += len;
}

float SystemConfigParaParser::getLimitPercent()
{
    HOY_SEMAPHORE_TAKE();
    float ret = ((((uint16_t)_payload[2]) << 8) | _payload[3]) / 10.0;
    HOY_SEMAPHORE_GIVE();
    return ret;
}

void SystemConfigParaParser::setLimitPercent(float value)
{
    HOY_SEMAPHORE_TAKE();
    _payload[2] = ((uint16_t)(value * 10)) >> 8;
    _payload[3] = ((uint16_t)(value * 10));
    HOY_SEMAPHORE_GIVE();
}

void SystemConfigParaParser::setLastLimitCommandSuccess(LastCommandSuccess status)
{
    _lastLimitCommandSuccess = status;
}

LastCommandSuccess SystemConfigParaParser::getLastLimitCommandSuccess()
{
    return _lastLimitCommandSuccess;
}

uint32_t SystemConfigParaParser::getLastUpdateCommand()
{
    return _lastUpdateCommand;
}

void SystemConfigParaParser::setLastUpdateCommand(uint32_t lastUpdate)
{
    _lastUpdateCommand = lastUpdate;
    setLastUpdate(lastUpdate);
}

void SystemConfigParaParser::setLastLimitRequestSuccess(LastCommandSuccess status)
{
    _lastLimitRequestSuccess = status;
}

LastCommandSuccess SystemConfigParaParser::getLastLimitRequestSuccess()
{
    return _lastLimitRequestSuccess;
}

uint32_t SystemConfigParaParser::getLastUpdateRequest()
{
    return _lastUpdateRequest;
}

void SystemConfigParaParser::setLastUpdateRequest(uint32_t lastUpdate)
{
    _lastUpdateRequest = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint8_t SystemConfigParaParser::getExpectedByteCount()
{
    return SYSTEM_CONFIG_PARA_SIZE;
}

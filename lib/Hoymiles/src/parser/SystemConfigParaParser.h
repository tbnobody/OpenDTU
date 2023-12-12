// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "Parser.h"

#define SYSTEM_CONFIG_PARA_SIZE 16

class SystemConfigParaParser : public Parser {
public:
    SystemConfigParaParser();
    void clearBuffer();
    void appendFragment(const uint8_t offset, const uint8_t* payload, const uint8_t len);

    float getLimitPercent() const;
    void setLimitPercent(const float value);

    void setLastLimitCommandSuccess(const LastCommandSuccess status);
    LastCommandSuccess getLastLimitCommandSuccess() const;
    uint32_t getLastUpdateCommand() const;
    void setLastUpdateCommand(const uint32_t lastUpdate);

    void setLastLimitRequestSuccess(const LastCommandSuccess status);
    LastCommandSuccess getLastLimitRequestSuccess() const;
    uint32_t getLastUpdateRequest() const;
    void setLastUpdateRequest(const uint32_t lastUpdate);

    // Returns 1 based amount of expected bytes of data
    uint8_t getExpectedByteCount() const;

private:
    uint8_t _payload[SYSTEM_CONFIG_PARA_SIZE];
    uint8_t _payloadLength;

    LastCommandSuccess _lastLimitCommandSuccess = CMD_OK; // Set to OK because we have to assume nothing is done at startup
    LastCommandSuccess _lastLimitRequestSuccess = CMD_NOK; // Set to NOK to fetch at startup

    uint32_t _lastUpdateCommand = 0;
    uint32_t _lastUpdateRequest = 0;
};
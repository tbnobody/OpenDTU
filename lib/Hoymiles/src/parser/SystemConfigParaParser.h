#pragma once
#include "Parser.h"
#include <Arduino.h>

#define SYSTEM_CONFIG_PARA_SIZE 16

class SystemConfigParaParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    float getLimitPercent();

    void setLastLimitCommandSuccess(LastCommandSuccess status);
    LastCommandSuccess getLastLimitCommandSuccess();

    void setLastLimitRequestSuccess(LastCommandSuccess status);
    LastCommandSuccess getLastLimitRequestSuccess();

private:
    uint8_t _payload[SYSTEM_CONFIG_PARA_SIZE];
    uint8_t _payloadLength;

    LastCommandSuccess _lastLimitCommandSuccess = CMD_OK; // Set to OK because we have to assume nothing is done at startup
    LastCommandSuccess _lastLimitRequestSuccess = CMD_NOK; // Set to NOK to fetch at startup
};
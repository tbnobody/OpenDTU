#pragma once
#include "Parser.h"
#include <Arduino.h>

#define SYSTEM_CONFIG_PARA_SIZE 16

class SystemConfigParaParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    float getLimitPercent();

private:
    uint8_t _payload[SYSTEM_CONFIG_PARA_SIZE];
    uint8_t _payloadLength;
};
#include "SystemConfigParaParser.h"
#include <cstring>

void SystemConfigParaParser::clearBuffer()
{
    memset(_payload, 0, SYSTEM_CONFIG_PARA_SIZE);
    _payloadLength = 0;
}

void SystemConfigParaParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > (SYSTEM_CONFIG_PARA_SIZE)) {
        Serial.printf("FATAL: (%s, %d) stats packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payload[offset], payload, len);
    _payloadLength += len;
}

float SystemConfigParaParser::getLimitPercent()
{
    return ((((uint16_t)_payload[2]) << 8) | _payload[3]) / 10;
}
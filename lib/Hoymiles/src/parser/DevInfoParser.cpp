#include "DevInfoParser.h"
#include <cstring>

void DevInfoParser::clearBuffer()
{
    memset(_payloadDevInfo, 0, DEV_INFO_SIZE);
    _devInfoLength = 0;
}

void DevInfoParser::appendFragment(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > DEV_INFO_SIZE) {
        Serial.printf("FATAL: (%s, %d) stats packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadDevInfo[offset], payload, len);
    _devInfoLength += len;
}
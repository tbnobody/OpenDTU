#include "DevInfoParser.h"
#include <cstring>
#include <HardwareSerial.h>

void DevInfoParser::clearBufferAll()
{
    memset(_payloadDevInfoAll, 0, DEV_INFO_SIZE);
    _devInfoAllLength = 0;
}

void DevInfoParser::appendFragmentAll(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > DEV_INFO_SIZE) {
        Serial.printf("FATAL: (%s, %d) dev info all packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadDevInfoAll[offset], payload, len);
    _devInfoAllLength += len;
}

void DevInfoParser::clearBufferSample()
{
    memset(_payloadDevInfoSample, 0, DEV_INFO_SIZE);
    _devInfoSampleLength = 0;
}

void DevInfoParser::appendFragmentSample(uint8_t offset, uint8_t* payload, uint8_t len)
{
    if (offset + len > DEV_INFO_SIZE) {
        Serial.printf("FATAL: (%s, %d) dev info Sample packet too large for buffer\n", __FILE__, __LINE__);
        return;
    }
    memcpy(&_payloadDevInfoSample[offset], payload, len);
    _devInfoSampleLength += len;
}

uint32_t DevInfoParser::getLastUpdateAll()
{
    return _lastUpdateAll;
}

void DevInfoParser::setLastUpdateAll(uint32_t lastUpdate)
{
    _lastUpdateAll = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint32_t DevInfoParser::getLastUpdateSample()
{
    return _lastUpdateSample;
}

void DevInfoParser::setLastUpdateSample(uint32_t lastUpdate)
{
    _lastUpdateSample = lastUpdate;
    setLastUpdate(lastUpdate);
}

uint16_t DevInfoParser::getFwBuildVersion()
{
    return (((uint16_t)_payloadDevInfoAll[0]) << 8) | _payloadDevInfoAll[1];
}

time_t DevInfoParser::getFwBuildDateTime()
{
    struct tm timeinfo = { 0 };
    timeinfo.tm_year = ((((uint16_t)_payloadDevInfoAll[2]) << 8) | _payloadDevInfoAll[3]) - 1900;

    timeinfo.tm_mon = ((((uint16_t)_payloadDevInfoAll[4]) << 8) | _payloadDevInfoAll[5]) / 100 - 1;
    timeinfo.tm_mday = ((((uint16_t)_payloadDevInfoAll[4]) << 8) | _payloadDevInfoAll[5]) % 100;

    timeinfo.tm_hour = ((((uint16_t)_payloadDevInfoAll[6]) << 8) | _payloadDevInfoAll[7]) / 100;
    timeinfo.tm_min = ((((uint16_t)_payloadDevInfoAll[6]) << 8) | _payloadDevInfoAll[7]) % 100;

    return timegm(&timeinfo);
}

uint16_t DevInfoParser::getFwBootloaderVersion()
{
    return (((uint16_t)_payloadDevInfoAll[8]) << 8) | _payloadDevInfoAll[9];
}

uint32_t DevInfoParser::getHwPartNumber()
{
    uint16_t hwpn_h;
    uint16_t hwpn_l;

    hwpn_h = (((uint16_t)_payloadDevInfoSample[2]) << 8) | _payloadDevInfoSample[3];
    hwpn_l = (((uint16_t)_payloadDevInfoSample[4]) << 8) | _payloadDevInfoSample[5];

    return ((uint32_t)hwpn_h << 16) | ((uint32_t)hwpn_l);
}

uint16_t DevInfoParser::getHwVersion()
{
    return (((uint16_t)_payloadDevInfoSample[6]) << 8) | _payloadDevInfoSample[7];
}

/* struct tm to seconds since Unix epoch */
time_t DevInfoParser::timegm(struct tm* t)
{
    register uint32_t year;
    register time_t result;
#define MONTHSPERYEAR 12 /* months per calendar year */
    static const int cumdays[MONTHSPERYEAR] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    /*@ +matchanyintegral @*/
    year = 1900 + t->tm_year + t->tm_mon / MONTHSPERYEAR;
    result = (year - 1970) * 365 + cumdays[t->tm_mon % MONTHSPERYEAR];
    result += (year - 1968) / 4;
    result -= (year - 1900) / 100;
    result += (year - 1600) / 400;
    if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0) && (t->tm_mon % MONTHSPERYEAR) < 2)
        result--;
    result += t->tm_mday - 1;
    result *= 24;
    result += t->tm_hour;
    result *= 60;
    result += t->tm_min;
    result *= 60;
    result += t->tm_sec;
    if (t->tm_isdst == 1)
        result -= 3600;
    /*@ -matchanyintegral @*/
    return (result);
}
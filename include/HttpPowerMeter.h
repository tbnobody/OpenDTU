// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

class HttpPowerMeterClass {
public:
    void init();
    bool updateValues();
    float getPower(int8_t phase);
    bool httpRequest(const char* url, const char* httpHeader, const char* httpValue, uint32_t timeout,
        char* response, size_t responseSize, char* error, size_t errorSize);
    float getFloatValueByJsonPath(const char* jsonString, const char* jsonPath, float &value);

private:
     float power[POWERMETER_MAX_PHASES];
};

extern HttpPowerMeterClass HttpPowerMeter;

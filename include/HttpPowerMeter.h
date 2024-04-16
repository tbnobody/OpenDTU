// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"

using Auth_t = PowerMeterHttpConfig::Auth;

class HttpPowerMeterClass {
public:
    void init();
    bool updateValues();
    float getPower(int8_t phase);
    char httpPowerMeterError[256];
    bool queryPhase(int phase, PowerMeterHttpConfig const& config);

private:
    float power[POWERMETER_MAX_PHASES];
    HTTPClient httpClient;
    String httpResponse;
    bool httpRequest(int phase, WiFiClient &wifiClient, const String& host, uint16_t port, const String& uri, bool https, PowerMeterHttpConfig const& config);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t, String& _base64Authorization);
    String extractParam(String& authReq, const String& param, const char delimit);
    String getcNonce(const int len);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter);
    bool tryGetFloatValueForPhase(int phase, const char* jsonPath);
    void prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue);
    String sha256(const String& data);
};

extern HttpPowerMeterClass HttpPowerMeter;

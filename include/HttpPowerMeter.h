// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>

class HttpPowerMeterClass {
public:
    void init();
    bool updateValues();
    float getPower(int8_t phase);
    char httpPowerMeterError[256];
    bool queryPhase(int phase, const String& urlProtocol, const String& urlHostname, const String& uri, Auth authType, const char* username, const char* password, 
        const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath);
    void extractUrlComponents(const String& url, String& protocol, String& hostname, String& uri);

private:    
    float power[POWERMETER_MAX_PHASES];
    HTTPClient httpClient;
    String httpResponse;
    bool httpRequest(int phase, WiFiClient &wifiClient, const String& urlProtocol, const String& urlHostname, const String& urlUri, Auth authType, const char* username,
           const char* password, const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath);
    String extractParam(String& authReq, const String& param, const char delimit);
    void getcNonce(char* cNounce);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter);
    bool tryGetFloatValueForPhase(int phase, int httpCode, const char* jsonPath);
    void prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue);    
    String sha256(const String& data);    
};

extern HttpPowerMeterClass HttpPowerMeter;

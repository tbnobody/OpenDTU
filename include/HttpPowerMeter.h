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
    bool queryPhase(int phase, const String& url, Auth authType, const char* username, const char* password, 
        const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath);


private:    
    float power[POWERMETER_MAX_PHASES];
    HTTPClient httpClient;
    String httpResponse;
    bool httpRequest(int phase, WiFiClient &wifiClient, const String& host, uint16_t port, const String& uri, bool https, Auth authType, const char* username,
           const char* password, const char* httpHeader, const char* httpValue, uint32_t timeout, const char* jsonPath);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t, String& _base64Authorization);
    String extractParam(String& authReq, const String& param, const char delimit);
    String getcNonce(const int len);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter);
    bool tryGetFloatValueForPhase(int phase, int httpCode, const char* jsonPath);
    void prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue);    
    String sha256(const String& data);    
};

extern HttpPowerMeterClass HttpPowerMeter;

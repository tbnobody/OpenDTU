// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"
#include "PowerMeterProvider.h"

using Auth_t = HttpRequestConfig::Auth;
using Unit_t = PowerMeterHttpJsonConfig::Unit;

class PowerMeterHttpJson : public PowerMeterProvider {
public:
    ~PowerMeterHttpJson();

    bool init() final { return true; }
    void loop() final;
    float getPowerTotal() const final;
    void doMqttPublish() const final;

    bool queryValue(int phase, PowerMeterHttpJsonConfig const& config);
    char httpPowerMeterError[256];
    float getCached(size_t idx) { return _cache[idx]; }

private:
    uint32_t _lastPoll;
    std::array<float, POWERMETER_HTTP_JSON_MAX_VALUES> _cache;
    std::array<float, POWERMETER_HTTP_JSON_MAX_VALUES> _powerValues;
    std::unique_ptr<WiFiClient> wifiClient;
    std::unique_ptr<HTTPClient> httpClient;
    String httpResponse;

    bool httpRequest(int phase, const String& host, uint16_t port, const String& uri, bool https, PowerMeterHttpJsonConfig const& config);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t);
    String extractParam(String& authReq, const String& param, const char delimit);
    String getcNonce(const int len);
    String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter);
    bool tryGetFloatValueForPhase(int phase, String jsonPath, Unit_t unit, bool signInverted);
    void prepareRequest(uint32_t timeout, const char* httpHeader, const char* httpValue);
    String sha256(const String& data);
};

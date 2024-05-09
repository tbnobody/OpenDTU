// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"
#include "PowerMeterSml.h"

class PowerMeterHttpSml : public PowerMeterSml {
public:
    ~PowerMeterHttpSml();

    bool init() final { return true; }
    void deinit() final { }
    void loop() final;
    bool updateValues();
    char tibberPowerMeterError[256];
    bool query(PowerMeterTibberConfig const& config);

private:
    uint32_t _lastPoll = 0;

    std::unique_ptr<WiFiClient> wifiClient;
    std::unique_ptr<HTTPClient> httpClient;
    String httpResponse;
    bool httpRequest(const String& host, uint16_t port, const String& uri, bool https, PowerMeterTibberConfig const& config);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t, String& _base64Authorization);
    void prepareRequest(uint32_t timeout);
};

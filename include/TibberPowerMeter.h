// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"

class TibberPowerMeterClass {
public:
    bool updateValues();
    char tibberPowerMeterError[256];
    bool query(PowerMeterTibberConfig const& config);

private:
    HTTPClient httpClient;
    String httpResponse;
    bool httpRequest(WiFiClient &wifiClient, const String& host, uint16_t port, const String& uri, bool https, PowerMeterTibberConfig const& config);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t, String& _base64Authorization);
    void prepareRequest(uint32_t timeout);
};

extern TibberPowerMeterClass TibberPowerMeter;

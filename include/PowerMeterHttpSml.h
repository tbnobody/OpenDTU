// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <list>
#include <mutex>
#include <memory>
#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"
#include "PowerMeterProvider.h"
#include "sml.h"

class PowerMeterHttpSml : public PowerMeterProvider {
public:
    ~PowerMeterHttpSml();

    bool init() final { return true; }
    void deinit() final { }
    void loop() final;
    float getPowerTotal() const final;
    void doMqttPublish() const final;
    bool updateValues();
    char tibberPowerMeterError[256];
    bool query(PowerMeterTibberConfig const& config);

private:
    mutable std::mutex _mutex;

    uint32_t _lastPoll = 0;

    float _activePower = 0.0;

    typedef struct {
        const unsigned char OBIS[6];
        void (*Fn)(double&);
        float* Arg;
    } OBISHandler;

    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_activePower}
    };

    std::unique_ptr<WiFiClient> wifiClient;
    std::unique_ptr<HTTPClient> httpClient;
    String httpResponse;
    bool httpRequest(const String& host, uint16_t port, const String& uri, bool https, PowerMeterTibberConfig const& config);
    bool extractUrlComponents(String url, String& _protocol, String& _hostname, String& _uri, uint16_t& uint16_t, String& _base64Authorization);
    void prepareRequest(uint32_t timeout);
};

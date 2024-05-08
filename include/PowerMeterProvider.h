// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"

class PowerMeterProvider {
public:
    virtual ~PowerMeterProvider() { }

    enum class Type : unsigned {
        MQTT = 0,
        SDM1PH = 1,
        SDM3PH = 2,
        HTTP = 3,
        SML = 4,
        SMAHM2 = 5,
        TIBBER = 6
    };

    // returns true if the provider is ready for use, false otherwise
    virtual bool init() = 0;

    virtual void deinit() = 0;
    virtual void loop() = 0;
    virtual float getPowerTotal() const = 0;

    uint32_t getLastUpdate() const { return _lastUpdate; }
    bool isDataValid() const;
    void mqttLoop() const;

protected:
    PowerMeterProvider() {
        auto const& config = Configuration.get();
        _verboseLogging = config.PowerMeter.VerboseLogging;
    }

    void gotUpdate() { _lastUpdate = millis(); }

    bool _verboseLogging;

private:
    virtual void doMqttPublish() const = 0;

    uint32_t _lastUpdate = 0;
    mutable uint32_t _lastMqttPublish = 0;
};

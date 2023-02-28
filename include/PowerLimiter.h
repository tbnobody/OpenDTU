// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

class PowerLimiterClass {
public:
    void init();
    void loop();
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

private:
    uint32_t _lastCommandSent;
    uint32_t _lastLoop;
    uint32_t _lastPowerMeterUpdate;
    uint16_t _lastRequestedPowerLimit;
    bool _consumeSolarPowerOnly;

    float _powerMeter1Power;
    float _powerMeter2Power;
    float _powerMeter3Power;

    bool canUseDirectSolarPower();
    float getDirectSolarPower();
    float getLoadCorrectedVoltage(std::shared_ptr<InverterAbstract> inverter);
    bool isStartThresholdReached(std::shared_ptr<InverterAbstract> inverter);
    bool isStopThresholdReached(std::shared_ptr<InverterAbstract> inverter);
};

extern PowerLimiterClass PowerLimiter;

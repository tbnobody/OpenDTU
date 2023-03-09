// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

enum PowerLimiterStates {
    STATE_DISCOVER = 0, 
    STATE_OFF, 
    STATE_CONSUME_SOLAR_POWER_ONLY, 
    STATE_NORMAL_OPERATION
};
   

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
    u_int8_t _plState = STATE_DISCOVER; 

    float _powerMeter1Power;
    float _powerMeter2Power;
    float _powerMeter3Power;

    bool canUseDirectSolarPower();
    int32_t calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool consumeSolarPowerOnly);
    void setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, uint32_t newPowerLimit);
    uint16_t getDirectSolarPower();
    float getLoadCorrectedVoltage(std::shared_ptr<InverterAbstract> inverter);
    bool isStartThresholdReached(std::shared_ptr<InverterAbstract> inverter);
    bool isStopThresholdReached(std::shared_ptr<InverterAbstract> inverter);
};

extern PowerLimiterClass PowerLimiter;

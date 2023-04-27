// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

typedef enum {
    SHUTDOWN = 0, 
    ACTIVE
} plStates;

typedef enum {
    EMPTY_WHEN_FULL= 0, 
    EMPTY_AT_NIGHT
} batDrainStrategy;
   

class PowerLimiterClass {
public:
    void init();
    void loop();
    plStates getPowerLimiterState();
    int32_t getLastRequestedPowewrLimit();
    void setDisable(bool disable);
    bool getDisable();

private:
    uint32_t _lastLoop = 0;
    int32_t _lastRequestedPowerLimit = 0;
    uint32_t _lastLimitSetTime = 0;
    plStates _plState = ACTIVE; 
    bool _disabled = false;
    bool _batteryDischargeEnabled = false;

    float _powerMeter1Power;
    float _powerMeter2Power;
    float _powerMeter3Power;

    bool canUseDirectSolarPower();
    int32_t calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled);
    void setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit);
    int32_t getDirectSolarPower();
    float getLoadCorrectedVoltage(std::shared_ptr<InverterAbstract> inverter);
    bool isStartThresholdReached(std::shared_ptr<InverterAbstract> inverter);
    bool isStopThresholdReached(std::shared_ptr<InverterAbstract> inverter);
};

extern PowerLimiterClass PowerLimiter;

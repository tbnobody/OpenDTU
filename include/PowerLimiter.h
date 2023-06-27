// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

#define PL_UI_STATE_INACTIVE 0
#define PL_UI_STATE_CHARGING 1
#define PL_UI_STATE_USE_SOLAR_ONLY 2
#define PL_UI_STATE_USE_SOLAR_AND_BATTERY 3

#define PL_MODE_ENABLE_NORMAL_OP 0
#define PL_MODE_FULL_DISABLE 1
#define PL_MODE_SOLAR_PT_ONLY 2

typedef enum {
    EMPTY_WHEN_FULL= 0, 
    EMPTY_AT_NIGHT
} batDrainStrategy;
   

class PowerLimiterClass {
public:
    enum class Status : unsigned {
        Initializing,
        DisabledByConfig,
        DisabledByMqtt,
        PowerMeterDisabled,
        PowerMeterTimeout,
        PowerMeterPending,
        InverterInvalid,
        InverterOffline,
        InverterCommandsDisabled,
        InverterLimitPending,
        InverterPowerCmdPending,
        InverterStatsPending,
        Settling,
        LowerLimitUndercut
    };

    void init();
    void loop();
    uint8_t getPowerLimiterState();
    int32_t getLastRequestedPowerLimit();
    void setMode(uint8_t mode);
    bool getMode();
    void calcNextInverterRestart();

private:
    enum class plStates : unsigned {
        INIT, // looping for the first time after system startup
        ACTIVE, // normal operation, sending power limit updates to inverter
        SHUTDOWN, // power limiter shuts down inverter
        OFF // inverter was shut down, power limiter is NOT active
    };

    int32_t _lastRequestedPowerLimit = 0;
    plStates _plState = plStates::INIT;
    Status _lastStatus = Status::Initializing;
    uint32_t _lastStatusPrinted = 0;
    uint8_t _mode = PL_MODE_ENABLE_NORMAL_OP;
    bool _batteryDischargeEnabled = false;
    uint32_t _nextInverterRestart = 0; // Values: 0->not calculated / 1->no restart configured / >1->time of next inverter restart in millis()
    uint32_t _nextCalculateCheck = 5000; // time in millis for next NTP check to calulate restart
    bool _fullSolarPassThroughEnabled = false;

    std::string const& getStatusText(Status status);
    void announceStatus(Status status);
    void shutdown(Status status);
    bool canUseDirectSolarPower();
    int32_t calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled);
    void commitPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t limit, bool enablePowerProduction);
    void setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit);
    int32_t getSolarChargePower();
    float getLoadCorrectedVoltage(std::shared_ptr<InverterAbstract> inverter);
    bool isStartThresholdReached(std::shared_ptr<InverterAbstract> inverter);
    bool isStopThresholdReached(std::shared_ptr<InverterAbstract> inverter);
    bool useFullSolarPassthrough(std::shared_ptr<InverterAbstract> inverter);
};

extern PowerLimiterClass PowerLimiter;

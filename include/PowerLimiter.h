// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>
#include <functional>

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
        WaitingForValidTimestamp,
        PowerMeterDisabled,
        PowerMeterTimeout,
        PowerMeterPending,
        InverterInvalid,
        InverterChanged,
        InverterOffline,
        InverterCommandsDisabled,
        InverterLimitPending,
        InverterPowerCmdPending,
        InverterDevInfoPending,
        InverterStatsPending,
        UnconditionalSolarPassthrough,
        NoVeDirect,
        Settling,
        Stable,
    };

    void init();
    void loop();
    uint8_t getPowerLimiterState();
    int32_t getLastRequestedPowerLimit();
    void setMode(uint8_t mode);
    bool getMode();
    void calcNextInverterRestart();

private:
    int32_t _lastRequestedPowerLimit = 0;
    uint32_t _shutdownTimeout = 0;
    Status _lastStatus = Status::Initializing;
    uint32_t _lastStatusPrinted = 0;
    uint32_t _lastCalculation = 0;
    static constexpr uint32_t _calculationBackoffMsDefault = 128;
    uint32_t _calculationBackoffMs = _calculationBackoffMsDefault;
    uint8_t _mode = PL_MODE_ENABLE_NORMAL_OP;
    std::shared_ptr<InverterAbstract> _inverter = nullptr;
    bool _batteryDischargeEnabled = false;
    uint32_t _nextInverterRestart = 0; // Values: 0->not calculated / 1->no restart configured / >1->time of next inverter restart in millis()
    uint32_t _nextCalculateCheck = 5000; // time in millis for next NTP check to calulate restart
    bool _fullSolarPassThroughEnabled = false;
    bool _verboseLogging = true;

    std::string const& getStatusText(Status status);
    void announceStatus(Status status);
    bool shutdown(Status status);
    bool shutdown() { return shutdown(_lastStatus); }
    int32_t inverterPowerDcToAc(std::shared_ptr<InverterAbstract> inverter, int32_t dcPower);
    void unconditionalSolarPassthrough(std::shared_ptr<InverterAbstract> inverter);
    bool canUseDirectSolarPower();
    int32_t calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled);
    void commitPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t limit, bool enablePowerProduction);
    bool setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit);
    int32_t getSolarChargePower();
    float getLoadCorrectedVoltage();
    bool testThreshold(float socThreshold, float voltThreshold,
            std::function<bool(float, float)> compare);
    bool isStartThresholdReached();
    bool isStopThresholdReached();
    bool isBelowStopThreshold();
    bool useFullSolarPassthrough();
};

extern PowerLimiterClass PowerLimiter;

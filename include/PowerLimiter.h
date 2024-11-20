// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "PowerLimiterInverter.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <atomic>
#include <deque>
#include <memory>
#include <functional>
#include <optional>
#include <TaskSchedulerDeclarations.h>
#include <frozen/string.h>

#define PL_UI_STATE_INACTIVE 0
#define PL_UI_STATE_CHARGING 1
#define PL_UI_STATE_USE_SOLAR_ONLY 2
#define PL_UI_STATE_USE_SOLAR_AND_BATTERY 3

class PowerLimiterClass {
public:
    PowerLimiterClass() = default;

    enum class Status : unsigned {
        Initializing,
        DisabledByConfig,
        DisabledByMqtt,
        WaitingForValidTimestamp,
        PowerMeterPending,
        InverterInvalid,
        InverterCmdPending,
        ConfigReload,
        InverterStatsPending,
        FullSolarPassthrough,
        UnconditionalSolarPassthrough,
        Stable,
    };

    void init(Scheduler& scheduler);
    void triggerReloadingConfig() { _reloadConfigFlag = true; }
    uint8_t getInverterUpdateTimeouts() const;
    uint8_t getPowerLimiterState();
    int32_t getInverterOutput() { return _lastExpectedInverterOutput; }
    bool getFullSolarPassThroughEnabled() const { return _fullSolarPassThroughEnabled; }

    enum class Mode : unsigned {
        Normal = 0,
        Disabled = 1,
        UnconditionalFullSolarPassthrough = 2
    };

    void setMode(Mode m) { _mode = m; }
    Mode getMode() const { return _mode; }
    bool usesBatteryPoweredInverter();
    bool isGovernedInverterProducing();

private:
    void loop();

    Task _loopTask;

    std::atomic<bool> _reloadConfigFlag = true;
    uint16_t _lastExpectedInverterOutput = 0;
    Status _lastStatus = Status::Initializing;
    uint32_t _lastStatusPrinted = 0;
    uint32_t _lastCalculation = 0;
    static constexpr uint32_t _calculationBackoffMsDefault = 128;
    uint32_t _calculationBackoffMs = _calculationBackoffMsDefault;
    Mode _mode = Mode::Normal;

    std::deque<std::unique_ptr<PowerLimiterInverter>> _inverters;
    bool _batteryDischargeEnabled = false;
    bool _nighttimeDischarging = false;
    std::pair<bool, uint32_t> _nextInverterRestart = { false, 0 };
    bool _fullSolarPassThroughEnabled = false;
    bool _verboseLogging = true;

    frozen::string const& getStatusText(Status status);
    void announceStatus(Status status);
    bool shutdown(Status status);
    void reloadConfig();
    std::pair<float, char const*> getInverterDcVoltage();
    float getBatteryVoltage(bool log = false);
    uint16_t solarDcToInverterAc(uint16_t dcPower);
    void fullSolarPassthrough(PowerLimiterClass::Status reason);
    int16_t calcHouseholdConsumption();
    using inverter_filter_t = std::function<bool(PowerLimiterInverter const&)>;
    uint16_t updateInverterLimits(uint16_t powerRequested, inverter_filter_t filter, std::string const& filterExpression);
    uint16_t calcBatteryAllowance(uint16_t powerRequested);
    bool updateInverters();
    uint16_t getSolarPassthroughPower();
    std::optional<uint16_t> getBatteryDischargeLimit();
    float getBatteryInvertersOutputAcWatts();
    float getLoadCorrectedVoltage();
    bool testThreshold(float socThreshold, float voltThreshold,
            std::function<bool(float, float)> compare);
    bool isStartThresholdReached();
    bool isStopThresholdReached();
    bool isBelowStopThreshold();
    void calcNextInverterRestart();
    bool isFullSolarPassthroughActive();
};

extern PowerLimiterClass PowerLimiter;

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */

#include "Battery.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <VeDirectFrameHandler.h>
#include "MessageOutput.h"
#include <ctime>
#include <cmath>
#include <map>

PowerLimiterClass PowerLimiter;

#define POWER_LIMITER_DEBUG

void PowerLimiterClass::init() { }

std::string const& PowerLimiterClass::getStatusText(PowerLimiterClass::Status status)
{
    static const std::string missing =  "programmer error: missing status text";

    static const std::map<Status, const std::string> texts = {
        { Status::Initializing, "initializing (should not see me)" },
        { Status::DisabledByConfig, "disabled by configuration" },
        { Status::DisabledByMqtt, "disabled by MQTT" },
        { Status::PowerMeterDisabled, "no power meter is configured/enabled" },
        { Status::PowerMeterTimeout, "power meter readings are outdated" },
        { Status::PowerMeterPending, "waiting for sufficiently recent power meter reading" },
        { Status::InverterInvalid, "invalid inverter selection/configuration" },
        { Status::InverterChanged, "target inverter changed" },
        { Status::InverterOffline, "inverter is offline (polling enabled? radio okay?)" },
        { Status::InverterCommandsDisabled, "inverter configuration prohibits sending commands" },
        { Status::InverterLimitPending, "waiting for a power limit command to complete" },
        { Status::InverterPowerCmdPending, "waiting for a start/stop/restart command to complete" },
        { Status::InverterStatsPending, "waiting for sufficiently recent inverter data" },
        { Status::UnconditionalSolarPassthrough, "unconditionally passing through all solar power (MQTT override)" },
        { Status::NoVeDirect, "VE.Direct disabled, connection broken, or data outdated" },
        { Status::Settling, "waiting for the system to settle" },
        { Status::Stable, "the system is stable, the last power limit is still valid" },
        { Status::LowerLimitUndercut, "calculated power limit undercuts configured lower limit" }
    };

    auto iter = texts.find(status);
    if (iter == texts.end()) { return missing; }

    return iter->second;
}

void PowerLimiterClass::announceStatus(PowerLimiterClass::Status status)
{
    // this method is called with high frequency. print the status text if
    // the status changed since we last printed the text of another one.
    // otherwise repeat the info with a fixed interval.
    if (_lastStatus == status && millis() < _lastStatusPrinted + 10 * 1000) { return; }

    // after announcing once that the DPL is disabled by configuration, it
    // should just be silent while it is disabled.
    if (status == Status::DisabledByConfig && _lastStatus == status) { return; }

    MessageOutput.printf("[%11.3f] DPL: %s\r\n",
        static_cast<double>(millis())/1000, getStatusText(status).c_str());

    _lastStatus = status;
    _lastStatusPrinted = millis();
}

void PowerLimiterClass::shutdown(PowerLimiterClass::Status status)
{
    announceStatus(status);

    if (_inverter == nullptr || !_inverter->isProducing() || !_inverter->isReachable()) {
        _inverter = nullptr;
        _shutdownInProgress = false;
        return;
    }

    _shutdownInProgress = true;

    auto lastLimitCommandState = _inverter->SystemConfigPara()->getLastLimitCommandSuccess();
    if (CMD_PENDING == lastLimitCommandState) { return; }

    auto lastPowerCommandState = _inverter->PowerCommand()->getLastPowerCommandSuccess();
    if (CMD_PENDING == lastPowerCommandState) { return; }

    CONFIG_T& config = Configuration.get();
    commitPowerLimit(_inverter, config.PowerLimiter_LowerPowerLimit, false);
}

void PowerLimiterClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (_shutdownInProgress) {
        // we transition from SHUTDOWN to OFF when we know the inverter was
        // shut down. until then, we retry shutting it down. in this case we
        // preserve the original status that lead to the decision to shut down.
        return shutdown(_lastStatus);
    }

    if (!config.PowerLimiter_Enabled) {
        return shutdown(Status::DisabledByConfig);
    }

    if (PL_MODE_FULL_DISABLE == _mode) {
        return shutdown(Status::DisabledByMqtt);
    }

    std::shared_ptr<InverterAbstract> currentInverter =
        Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);

    // in case of (newly) broken configuration, shut down
    // the last inverter we worked with (if any)
    if (currentInverter == nullptr) {
        return shutdown(Status::InverterInvalid);
    }

    // if the DPL is supposed to manage another inverter now, we first
    // shut down the previous one, if any. then we pick up the new one.
    if (_inverter != nullptr && _inverter->serial() != currentInverter->serial()) {
        return shutdown(Status::InverterChanged);
    }

    // update our pointer as the configuration might have changed
    _inverter = currentInverter;

    // data polling is disabled or the inverter is deemed offline
    if (!_inverter->isReachable()) {
        return announceStatus(Status::InverterOffline);
    }

    // sending commands to the inverter is disabled
    if (!_inverter->getEnableCommands()) {
        return announceStatus(Status::InverterCommandsDisabled);
    }

    // concerns active power commands (power limits) only (also from web app or MQTT)
    auto lastLimitCommandState = _inverter->SystemConfigPara()->getLastLimitCommandSuccess();
    if (CMD_PENDING == lastLimitCommandState) {
        return announceStatus(Status::InverterLimitPending);
    }

    // concerns power commands (start, stop, restart) only (also from web app or MQTT)
    auto lastPowerCommandState = _inverter->PowerCommand()->getLastPowerCommandSuccess();
    if (CMD_PENDING == lastPowerCommandState) {
        return announceStatus(Status::InverterPowerCmdPending);
    }

    if (PL_MODE_SOLAR_PT_ONLY == _mode) {
        // handle this mode of operation separately
        return unconditionalSolarPassthrough(_inverter);
    }

    // the normal mode of operation requires a valid
    // power meter reading to calculate a power limit
    if (!config.PowerMeter_Enabled) {
        return shutdown(Status::PowerMeterDisabled);
    }

    if (millis() - PowerMeter.getLastPowerMeterUpdate() > (30 * 1000)) {
        return shutdown(Status::PowerMeterTimeout);
    }

    // concerns both power limits and start/stop/restart commands and is
    // only updated if a respective response was received from the inverter
    auto lastUpdateCmd = std::max(
            _inverter->SystemConfigPara()->getLastUpdateCommand(),
            _inverter->PowerCommand()->getLastUpdateCommand());

    // wait for power meter and inverter stat updates after a settling phase
    auto settlingEnd = lastUpdateCmd + 3 * 1000;

    if (millis() < settlingEnd) { return announceStatus(Status::Settling); }

    if (_inverter->Statistics()->getLastUpdate() <= settlingEnd) {
        return announceStatus(Status::InverterStatsPending);
    }

    if (PowerMeter.getLastPowerMeterUpdate() <= settlingEnd) {
        return announceStatus(Status::PowerMeterPending);
    }

    // since _lastCalculation and _calculationBackoffMs are initialized to
    // zero, this test is passed the first time the condition is checked.
    if (millis() < (_lastCalculation + _calculationBackoffMs)) {
        return announceStatus(Status::Stable);
    }

#ifdef POWER_LIMITER_DEBUG
  MessageOutput.println("[PowerLimiterClass::loop] ******************* ENTER **********************");
#endif

    // Check if next inverter restart time is reached
    if ((_nextInverterRestart > 1) && (_nextInverterRestart <= millis())) {
        MessageOutput.println("[PowerLimiterClass::loop] send inverter restart");
        _inverter->sendRestartControlRequest();
        calcNextInverterRestart();
    }

    // Check if NTP time is set and next inverter restart not calculated yet
    if ((config.PowerLimiter_RestartHour >= 0)  && (_nextInverterRestart == 0) ) {
        // check every 5 seconds
        if (_nextCalculateCheck < millis()) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 5)) {
                calcNextInverterRestart();
            } else {
                MessageOutput.println("[PowerLimiterClass::loop] inverter restart calculation: NTP not ready");
                _nextCalculateCheck += 5000;
            }
        }
    }

    // Printout some stats
    if (millis() - PowerMeter.getLastPowerMeterUpdate() < (30 * 1000)) {
        float dcVoltage = _inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);
        MessageOutput.printf("[PowerLimiterClass::loop] dcVoltage: %.2f Voltage Start Threshold: %.2f Voltage Stop Threshold: %.2f inverter->isProducing(): %d\r\n",
            dcVoltage, config.PowerLimiter_VoltageStartThreshold, config.PowerLimiter_VoltageStopThreshold, _inverter->isProducing());
    }

    // Battery charging cycle conditions
    // First we always disable discharge if the battery is empty
    if (isStopThresholdReached(_inverter)) {
      // Disable battery discharge when empty
      _batteryDischargeEnabled = false;
    } else {
      // UI: Solar Passthrough Enabled -> false
      // Battery discharge can be enabled when start threshold is reached
      if (!config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached(_inverter)) {
        _batteryDischargeEnabled = true;
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_AT_NIGHT
      if (config.PowerLimiter_SolarPassThroughEnabled && config.PowerLimiter_BatteryDrainStategy == EMPTY_AT_NIGHT) {
        if(isStartThresholdReached(_inverter)) {
            // In this case we should only discharge the battery as long it is above startThreshold
            _batteryDischargeEnabled = true;
        }
        else {
            // In this case we should only discharge the battery when there is no sunshine
            _batteryDischargeEnabled = !canUseDirectSolarPower();
        }
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_WHEN_FULL
      // Battery discharge can be enabled when start threshold is reached
      if (config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached(_inverter) && config.PowerLimiter_BatteryDrainStategy == EMPTY_WHEN_FULL) {
        _batteryDischargeEnabled = true;
      }
    }
    // Calculate and set Power Limit
    int32_t newPowerLimit = calcPowerLimit(_inverter, canUseDirectSolarPower(), _batteryDischargeEnabled);
    bool limitUpdated = setNewPowerLimit(_inverter, newPowerLimit);
#ifdef POWER_LIMITER_DEBUG
    MessageOutput.printf("[PowerLimiterClass::loop] Status: SolarPT enabled %i, Drain Strategy: %i, canUseDirectSolarPower: %i, Batt discharge: %i\r\n",
        config.PowerLimiter_SolarPassThroughEnabled, config.PowerLimiter_BatteryDrainStategy, canUseDirectSolarPower(), _batteryDischargeEnabled);
    MessageOutput.printf("[PowerLimiterClass::loop] Status: StartTH %i, StopTH: %i, loadCorrectedV %f\r\n",
        isStartThresholdReached(_inverter), isStopThresholdReached(_inverter), getLoadCorrectedVoltage(_inverter));
    MessageOutput.printf("[PowerLimiterClass::loop] Status Batt: Ena: %i, SOC: %i, StartTH: %i, StopTH: %i, LastUpdate: %li\r\n",
        config.Battery_Enabled, Battery.stateOfCharge, config.PowerLimiter_BatterySocStartThreshold, config.PowerLimiter_BatterySocStopThreshold, millis() - Battery.stateOfChargeLastUpdate);
    MessageOutput.printf("[PowerLimiterClass::loop] ******************* Leaving PL, PL set to: %i, SP: %i, Batt: %i, PM: %f\r\n", newPowerLimit, canUseDirectSolarPower(), _batteryDischargeEnabled, round(PowerMeter.getPowerTotal()));
#endif

    _lastCalculation = millis();

    if (!limitUpdated) {
        // increase polling backoff if system seems to be stable
        _calculationBackoffMs = std::min<uint32_t>(1024, _calculationBackoffMs * 2);
        return announceStatus(Status::Stable);
    }

    _calculationBackoffMs = 128;
}

/**
 * calculate the AC output power (limit) to set, such that the inverter uses
 * the given power on its DC side, i.e., adjust the power for the inverter's
 * efficiency.
 */
int32_t PowerLimiterClass::inverterPowerDcToAc(std::shared_ptr<InverterAbstract> inverter, int32_t dcPower)
{
    CONFIG_T& config = Configuration.get();

    float inverterEfficiencyPercent = inverter->Statistics()->getChannelFieldValue(
        TYPE_AC, static_cast<ChannelNum_t>(config.PowerLimiter_InverterChannelId), FLD_EFF);

    // fall back to hoymiles peak efficiency as per datasheet if inverter
    // is currently not producing (efficiency is zero in that case)
    float inverterEfficiencyFactor = (inverterEfficiencyPercent > 0) ? inverterEfficiencyPercent/100 : 0.967;

    return dcPower * inverterEfficiencyFactor;
}

/**
 * implements the "unconditional solar passthrough" mode of operation, which
 * can currently only be set using MQTT. in this mode of operation, the
 * inverter shall behave as if it was connected to the solar panels directly,
 * i.e., all solar power (and only solar power) is fed to the AC side,
 * independent from the power meter reading.
 */
void PowerLimiterClass::unconditionalSolarPassthrough(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    if (!config.Vedirect_Enabled || !VeDirect.isDataValid()) {
        return shutdown(Status::NoVeDirect);
    }

    int32_t solarPower = VeDirect.veFrame.V * VeDirect.veFrame.I;
    setNewPowerLimit(inverter, inverterPowerDcToAc(inverter, solarPower));
    announceStatus(Status::UnconditionalSolarPassthrough);
}

uint8_t PowerLimiterClass::getPowerLimiterState() {
    if (_inverter == nullptr || !_inverter->isReachable()) {
        return PL_UI_STATE_INACTIVE;
    }

    if (_inverter->isProducing() && _batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_AND_BATTERY;
    }

    if (_inverter->isProducing() && !_batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_ONLY;
    }

    if(!_inverter->isProducing()) {
       return PL_UI_STATE_CHARGING;
    }

    return PL_UI_STATE_INACTIVE;
}

int32_t PowerLimiterClass::getLastRequestedPowerLimit() {
    return _lastRequestedPowerLimit;
}

bool PowerLimiterClass::getMode() {
    return _mode;
}

void PowerLimiterClass::setMode(uint8_t mode) {
    _mode = mode;
}

bool PowerLimiterClass::canUseDirectSolarPower()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter_SolarPassThroughEnabled
            || !config.Vedirect_Enabled) {
        return false;
    }

    if (VeDirect.veFrame.PPV < 20) {
        // Not enough power
        return false;
    }

    return true;
}


// Logic table
// | Case # | batteryDischargeEnabled | solarPowerEnabled | useFullSolarPassthrough | Result                                                      |
// | 1      | false                   | false             | doesn't matter          | PL = 0                                                      |
// | 2      | false                   | true              | doesn't matter          | PL = Victron Power                                          |
// | 3      | true                    | doesn't matter    | false                   | PL = PowerMeter value (Battery can supply unlimited energy) |
// | 4      | true                    | false             | true                    | PL = PowerMeter value                                       |
// | 5      | true                    | true              | true                    | PL = max(PowerMeter value, Victron Power)                   |

int32_t PowerLimiterClass::calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled)
{
    CONFIG_T& config = Configuration.get();
    
    int32_t acPower = 0;
    int32_t newPowerLimit = round(PowerMeter.getPowerTotal());

    if (!solarPowerEnabled && !batteryDischargeEnabled) {
      // Case 1 - No energy sources available
      return 0;
    }

    if (config.PowerLimiter_IsInverterBehindPowerMeter) {
        // If the inverter the behind the power meter (part of measurement),
        // the produced power of this inverter has also to be taken into account.
        // We don't use FLD_PAC from the statistics, because that
        // data might be too old and unreliable.
        acPower = static_cast<int>(inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_PAC)); 
        newPowerLimit += acPower;
    }

    // We're not trying to hit 0 exactly but take an offset into account
    // This means we never fully compensate the used power with the inverter 
    // Case 3
    newPowerLimit -= config.PowerLimiter_TargetPowerConsumption;

    // At this point we've calculated the required energy to compensate for household consumption. 
    // If the battery is enabled this can always be supplied since we assume that the battery can supply unlimited power
    // The next step is to determine if the Solar power as provided by the Victron charger
    // actually constrains or dictates another inverter power value
    int32_t adjustedVictronChargePower = inverterPowerDcToAc(inverter, getSolarChargePower());

    // Battery can be discharged and we should output max (Victron solar power || power meter value)
    if(batteryDischargeEnabled && useFullSolarPassthrough(inverter)) {
      // Case 5
      newPowerLimit = newPowerLimit > adjustedVictronChargePower ? newPowerLimit : adjustedVictronChargePower;
    } 

    // We should use Victron solar power only (corrected by efficiency factor)
    if (solarPowerEnabled && !batteryDischargeEnabled) {
        // Case 2 - Limit power to solar power only
        MessageOutput.printf("[PowerLimiterClass::loop] Consuming Solar Power Only -> adjustedVictronChargePower: %d, powerConsumption: %d \r\n",
            adjustedVictronChargePower, newPowerLimit);

        newPowerLimit = std::min(newPowerLimit, adjustedVictronChargePower);
    }

    MessageOutput.printf("[PowerLimiterClass::loop] newPowerLimit: %d\r\n", newPowerLimit);
    return newPowerLimit;
}

void PowerLimiterClass::commitPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t limit, bool enablePowerProduction)
{
    // disable power production as soon as possible.
    // setting the power limit is less important.
    if (!enablePowerProduction && inverter->isProducing()) {
        MessageOutput.println("[PowerLimiterClass::commitPowerLimit] Stopping inverter...");
        inverter->sendPowerControlRequest(false);
    }

    inverter->sendActivePowerControlRequest(static_cast<float>(limit),
            PowerLimitControlType::AbsolutNonPersistent);

    _lastRequestedPowerLimit = limit;

    // enable power production only after setting the desired limit,
    // such that an older, greater limit will not cause power spikes.
    if (enablePowerProduction && !inverter->isProducing()) {
        MessageOutput.println("[PowerLimiterClass::commitPowerLimit] Starting up inverter...");
        inverter->sendPowerControlRequest(true);
    }
}

/**
 * enforces limits and a hystersis on the requested power limit, after scaling
 * the power limit to the ratio of total and producing inverter channels.
 * commits the sanitized power limit. returns true if a limit update was
 * committed, false otherwise.
 */
bool PowerLimiterClass::setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit)
{
    CONFIG_T& config = Configuration.get();

    // Stop the inverter if limit is below threshold.
    if (newPowerLimit < config.PowerLimiter_LowerPowerLimit) {
        shutdown(Status::LowerLimitUndercut);
        return true;
    }

    // enforce configured upper power limit
    int32_t effPowerLimit = std::min(newPowerLimit, config.PowerLimiter_UpperPowerLimit);


    // scale the power limit by the amount of all inverter channels devided by
    // the amount of producing inverter channels. the inverters limit each of
    // the n channels to 1/n of the total power limit. scaling the power limit
    // ensures the total inverter output is what we are asking for.
    std::list<ChannelNum_t> dcChnls = inverter->Statistics()->getChannelsByType(TYPE_DC);
    int dcProdChnls = 0, dcTotalChnls = dcChnls.size();
    for (auto& c : dcChnls) {
        if (inverter->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC) > 1.0) {
            dcProdChnls++;
        }
    }
    if (dcProdChnls > 0) {
        MessageOutput.printf("[PowerLimiterClass::setNewPowerLimit] %d channels total, %d producing channels, scaling power limit\r\n",
                dcTotalChnls, dcProdChnls);
        effPowerLimit = round(newPowerLimit * static_cast<float>(dcTotalChnls) / dcProdChnls);
        if (effPowerLimit > inverter->DevInfo()->getMaxPower()) {
            effPowerLimit = inverter->DevInfo()->getMaxPower();
        }
    }

    // Check if the new value is within the limits of the hysteresis
    auto diff = std::abs(effPowerLimit - _lastRequestedPowerLimit);
    if ( diff < config.PowerLimiter_TargetPowerConsumptionHysteresis) {
        MessageOutput.printf("[PowerLimiterClass::setNewPowerLimit] reusing old limit: %d W, diff: %d, hysteresis: %d\r\n",
                _lastRequestedPowerLimit, diff, config.PowerLimiter_TargetPowerConsumptionHysteresis);
        return false;
    }

    MessageOutput.printf("[PowerLimiterClass::setNewPowerLimit] using new limit: %d W, requested power limit: %d\r\n",
            effPowerLimit, newPowerLimit);

    commitPowerLimit(inverter, effPowerLimit, true);
    return true;
}

int32_t PowerLimiterClass::getSolarChargePower()
{
    if (!canUseDirectSolarPower()) {
        return 0;
    }

    return VeDirect.veFrame.V * VeDirect.veFrame.I;
}

float PowerLimiterClass::getLoadCorrectedVoltage(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    float acPower = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_PAC);
    float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC); 

    if (dcVoltage <= 0.0) {
        return 0.0;
    }

    return dcVoltage + (acPower * config.PowerLimiter_VoltageLoadCorrectionFactor);
}

bool PowerLimiterClass::isStartThresholdReached(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    // Check if the Battery interface is enabled and the SOC start threshold is reached
    if (config.Battery_Enabled
            && config.PowerLimiter_BatterySocStartThreshold > 0.0
            && (millis() - Battery.stateOfChargeLastUpdate) < 60000
            && Battery.stateOfCharge >= config.PowerLimiter_BatterySocStartThreshold) {
        return true;
    }

    // Otherwise we use the voltage threshold
    if (config.PowerLimiter_VoltageStartThreshold <= 0.0) {
        return false;
    }

    float correctedDcVoltage = getLoadCorrectedVoltage(inverter);
    return correctedDcVoltage >= config.PowerLimiter_VoltageStartThreshold;
}

bool PowerLimiterClass::isStopThresholdReached(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    // Check if the Battery interface is enabled and the SOC stop threshold is reached
    if (config.Battery_Enabled
            && config.PowerLimiter_BatterySocStopThreshold > 0.0
            && (millis() - Battery.stateOfChargeLastUpdate) < 60000
            && Battery.stateOfCharge <= config.PowerLimiter_BatterySocStopThreshold) {
        return true;
    }

    // Otherwise we use the voltage threshold
    if (config.PowerLimiter_VoltageStopThreshold <= 0.0) {
        return false;
    }

    float correctedDcVoltage = getLoadCorrectedVoltage(inverter);
    return correctedDcVoltage <= config.PowerLimiter_VoltageStopThreshold;
}

/// @brief calculate next inverter restart in millis
void PowerLimiterClass::calcNextInverterRestart()
{
    CONFIG_T& config = Configuration.get();

    // first check if restart is configured at all
    if (config.PowerLimiter_RestartHour < 0) {
        _nextInverterRestart = 1;
        MessageOutput.println("[PowerLimiterClass::calcNextInverterRestart] _nextInverterRestart disabled");
        return;
    }

    // read time from timeserver, if time is not synced then return
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        // calculation first step is offset to next restart in minutes
        uint16_t dayMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        uint16_t targetMinutes = config.PowerLimiter_RestartHour * 60;
        if (config.PowerLimiter_RestartHour > timeinfo.tm_hour) {
            // next restart is on the same day
            _nextInverterRestart = targetMinutes - dayMinutes;
        } else {
            // next restart is on next day
            _nextInverterRestart = 1440 - dayMinutes + targetMinutes;
        }
        #ifdef POWER_LIMITER_DEBUG
        MessageOutput.printf("[PowerLimiterClass::calcNextInverterRestart] Localtime read %d %d / configured RestartHour %d\r\n", timeinfo.tm_hour, timeinfo.tm_min, config.PowerLimiter_RestartHour);
        MessageOutput.printf("[PowerLimiterClass::calcNextInverterRestart] dayMinutes %d / targetMinutes %d\r\n", dayMinutes, targetMinutes);
        MessageOutput.printf("[PowerLimiterClass::calcNextInverterRestart] next inverter restart in %d minutes\r\n", _nextInverterRestart);
        #endif
        // then convert unit for next restart to milliseconds and add current uptime millis()
        _nextInverterRestart *= 60000;
        _nextInverterRestart += millis();
    } else {
        MessageOutput.println("[PowerLimiterClass::calcNextInverterRestart] getLocalTime not successful, no calculation");
        _nextInverterRestart = 0;
    }
    MessageOutput.printf("[PowerLimiterClass::calcNextInverterRestart] _nextInverterRestart @ %d millis\r\n", _nextInverterRestart);
}

bool PowerLimiterClass::useFullSolarPassthrough(std::shared_ptr<InverterAbstract> inverter)
{
    CONFIG_T& config = Configuration.get();

    // We only do full solar PT if general solar PT is enabled
    if(!config.PowerLimiter_SolarPassThroughEnabled) {
      return false;
    }

    // Check if the Battery interface is enabled and the SOC stop threshold is reached
    if (config.Battery_Enabled
            && config.PowerLimiter_FullSolarPassThroughSoc > 0.0
            && (millis() - Battery.stateOfChargeLastUpdate) < 60000
            && Battery.stateOfCharge >= config.PowerLimiter_FullSolarPassThroughSoc) {
        return true;
    }
    
    // Otherwise we use the voltage threshold
    if (config.PowerLimiter_FullSolarPassThroughStartVoltage <= 0.0 || config.PowerLimiter_FullSolarPassThroughStopVoltage <= 0.0) {
        return false;
    }

    float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);

#ifdef POWER_LIMITER_DEBUG
    MessageOutput.printf("[PowerLimiterClass::loop] useFullSolarPassthrough: FullSolarPT Start %f, FullSolarPT Stop: %f, dcVoltage: %f\r\n",
        config.PowerLimiter_FullSolarPassThroughStartVoltage, config.PowerLimiter_FullSolarPassThroughStopVoltage, dcVoltage);
#endif 

    if (dcVoltage <= 0.0) {
        return false;
    }
    
    if (dcVoltage >= config.PowerLimiter_FullSolarPassThroughStartVoltage) {
      _fullSolarPassThroughEnabled = true;
    }

    if (dcVoltage <= config.PowerLimiter_FullSolarPassThroughStopVoltage) {
      _fullSolarPassThroughEnabled = false;
    }

    return _fullSolarPassThroughEnabled;
}

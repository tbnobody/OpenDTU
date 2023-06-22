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

PowerLimiterClass PowerLimiter;

#define POWER_LIMITER_DEBUG

void PowerLimiterClass::init()
{
  CONFIG_T& config = Configuration.get();
  if (config.PowerLimiter_Enabled) {
    // We'll start in active state
    _plState = ACTIVE;
  } else {
    _plState = SHUTDOWN;
  }
}

void PowerLimiterClass::loop()
{
    CONFIG_T& config = Configuration.get();

    // Run inital checks to make sure we have met the basic conditions
    if (!config.PowerMeter_Enabled
            || !Hoymiles.isAllRadioIdle()
            || (millis() - _lastLoop) < (config.PowerLimiter_Interval * 1000)) {
        return;
    }

#ifdef POWER_LIMITER_DEBUG
  MessageOutput.println("[PowerLimiterClass::loop] ******************* ENTER **********************");
#endif

    _lastLoop = millis();

    std::shared_ptr<InverterAbstract> inverter = Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);
    if (inverter == nullptr || !inverter->isReachable()) {
#ifdef POWER_LIMITER_DEBUG
  MessageOutput.println("[PowerLimiterClass::loop] ******************* No inverter found");
#endif
        return;
    }

    // Check if next inverter restart time is reached
    if ((_nextInverterRestart > 1) && (_nextInverterRestart <= millis())) {
        MessageOutput.println("[PowerLimiterClass::loop] send inverter restart");
        inverter->sendRestartControlRequest();
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

    // Make sure inverter is turned off if PL is disabled by user/MQTT
    if (((!config.PowerLimiter_Enabled || _mode == PL_MODE_FULL_DISABLE) && _plState != SHUTDOWN)) {
        if (inverter->isProducing()) {
            MessageOutput.printf("PL initiated inverter shutdown.\r\n");
            inverter->sendActivePowerControlRequest(static_cast<float>(config.PowerLimiter_LowerPowerLimit), PowerLimitControlType::AbsolutNonPersistent);
            _lastRequestedPowerLimit = config.PowerLimiter_LowerPowerLimit;
            inverter->sendPowerControlRequest(false);
        } else {
            _plState = SHUTDOWN;
        }
#ifdef POWER_LIMITER_DEBUG
  MessageOutput.printf("[PowerLimiterClass::loop] ******************* PL put into shutdown, _plState = %i\r\n", _plState);
#endif        
        return;
    }

    // Return if power limiter is disabled
    if (!config.PowerLimiter_Enabled || _mode == PL_MODE_FULL_DISABLE) {
#ifdef POWER_LIMITER_DEBUG
  MessageOutput.printf("[PowerLimiterClass::loop] ******************* PL disabled\r\n");
#endif        
      return;
    }

    // Safety check, return on too old power meter values
    if (millis() - PowerMeter.getLastPowerMeterUpdate() > (30 * 1000)
            || (millis() - inverter->Statistics()->getLastUpdate()) > (config.Dtu_PollInterval * 10 * 1000)) {
        // If the power meter values are older than 30 seconds, 
        // or the Inverter Stats are older then 10x the poll interval
        // set the limit to lower power limit for safety reasons.
        MessageOutput.println("[PowerLimiterClass::loop] Power Meter/Inverter values too old. Using 0W (i.e. disable inverter)");
        inverter->sendActivePowerControlRequest(static_cast<float>(config.PowerLimiter_LowerPowerLimit), PowerLimitControlType::AbsolutNonPersistent);
        _lastRequestedPowerLimit = config.PowerLimiter_LowerPowerLimit;
        inverter->sendPowerControlRequest(false);
#ifdef POWER_LIMITER_DEBUG
  MessageOutput.printf("[PowerLimiterClass::loop] ******************* PL safety shutdown, update times exceeded PM: %li, Inverter: %li \r\n", millis() - PowerMeter.getLastPowerMeterUpdate(), millis() - inverter->Statistics()->getLastUpdate());
#endif        
        return;
    }

  	// At this point the PL is enabled but we could still be in the shutdown state 
    _plState = ACTIVE;

    // If the last inverter update was before the last limit updated, don't do anything.
    // Also give the Power meter 3 seconds time to recognize power changes after the last set limit
    // as the Hoymiles MPPT might not react immediately.
    if (inverter->Statistics()->getLastUpdate() <= _lastLimitSetTime
        || PowerMeter.getLastPowerMeterUpdate() <= (_lastLimitSetTime + 3000)) {
#ifdef POWER_LIMITER_DEBUG
  MessageOutput.printf("[PowerLimiterClass::loop] ******************* PL inverter updates PM: %i, Inverter: %i \r\n", PowerMeter.getLastPowerMeterUpdate() - (_lastLimitSetTime + 3000), inverter->Statistics()->getLastUpdate() - _lastLimitSetTime);
#endif                  
        return;
    }

    // Printout some stats
    if (millis() - PowerMeter.getLastPowerMeterUpdate() < (30 * 1000)) {
        float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);
        MessageOutput.printf("[PowerLimiterClass::loop] dcVoltage: %.2f Voltage Start Threshold: %.2f Voltage Stop Threshold: %.2f inverter->isProducing(): %d\r\n",
            dcVoltage, config.PowerLimiter_VoltageStartThreshold, config.PowerLimiter_VoltageStopThreshold, inverter->isProducing());
    }

    // Battery charging cycle conditions
    // First we always disable discharge if the battery is empty
    if (isStopThresholdReached(inverter)) {
      // Disable battery discharge when empty
      _batteryDischargeEnabled = false;
    } else {
      // UI: Solar Passthrough Enabled -> false
      // Battery discharge can be enabled when start threshold is reached
      if (!config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached(inverter)) {
        _batteryDischargeEnabled = true;
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_AT_NIGHT
      if (config.PowerLimiter_SolarPassThroughEnabled && config.PowerLimiter_BatteryDrainStategy == EMPTY_AT_NIGHT) {
        if(isStartThresholdReached(inverter)) {
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
      if (config.PowerLimiter_SolarPassThroughEnabled && isStartThresholdReached(inverter) && config.PowerLimiter_BatteryDrainStategy == EMPTY_WHEN_FULL) {
        _batteryDischargeEnabled = true;
      }
    }
    // Calculate and set Power Limit
    int32_t newPowerLimit = calcPowerLimit(inverter, canUseDirectSolarPower(), _batteryDischargeEnabled);
    setNewPowerLimit(inverter, newPowerLimit);
#ifdef POWER_LIMITER_DEBUG
    MessageOutput.printf("[PowerLimiterClass::loop] Status: SolarPT enabled %i, Drain Strategy: %i, canUseDirectSolarPower: %i, Batt discharge: %i\r\n",
        config.PowerLimiter_SolarPassThroughEnabled, config.PowerLimiter_BatteryDrainStategy, canUseDirectSolarPower(), _batteryDischargeEnabled);
    MessageOutput.printf("[PowerLimiterClass::loop] Status: StartTH %i, StopTH: %i, loadCorrectedV %f\r\n",
        isStartThresholdReached(inverter), isStopThresholdReached(inverter), getLoadCorrectedVoltage(inverter));
    MessageOutput.printf("[PowerLimiterClass::loop] Status Batt: Ena: %i, SOC: %i, StartTH: %i, StopTH: %i, LastUpdate: %li\r\n",
        config.Battery_Enabled, Battery.stateOfCharge, config.PowerLimiter_BatterySocStartThreshold, config.PowerLimiter_BatterySocStopThreshold, millis() - Battery.stateOfChargeLastUpdate);
    MessageOutput.printf("[PowerLimiterClass::loop] ******************* Leaving PL, PL set to: %i, SP: %i, Batt: %i, PM: %f\r\n", newPowerLimit, canUseDirectSolarPower(), _batteryDischargeEnabled, round(PowerMeter.getPowerTotal()));
#endif
}

uint8_t PowerLimiterClass::getPowerLimiterState() {
    CONFIG_T& config = Configuration.get();

    std::shared_ptr<InverterAbstract> inverter = Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);
    if (inverter == nullptr || !inverter->isReachable()) {
        return PL_UI_STATE_INACTIVE;
    }

    if (inverter->isProducing() && _batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_AND_BATTERY;
    }

    if (inverter->isProducing() && !_batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_ONLY;
    }

    if(!inverter->isProducing()) {
       return PL_UI_STATE_CHARGING;
    }

    return PL_UI_STATE_INACTIVE;
}

int32_t PowerLimiterClass::getLastRequestedPowewrLimit() {
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

    // Check if the new value is within the limits of the hysteresis and
    // if we can discharge the battery
    // If things did not change much we just use the old setting
    if ((newPowerLimit - acPower) >= (-config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
        (newPowerLimit - acPower) <= (+config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
        batteryDischargeEnabled ) {
            MessageOutput.println("[PowerLimiterClass::loop] reusing old limit");
            return _lastRequestedPowerLimit;
    }

    // At this point we've calculated the required energy to compensate for household consumption. 
    // If the battery is enabled this can always be supplied since we assume that the battery can supply unlimited power
    // The next step is to determine if the Solar power as provided by the Victron charger
    // actually constrains or dictates another inverter power value
    float inverterEfficiencyPercent = inverter->Statistics()->getChannelFieldValue(
        TYPE_AC, static_cast<ChannelNum_t>(config.PowerLimiter_InverterChannelId), FLD_EFF);
    // fall back to hoymiles peak efficiency as per datasheet if inverter
    // is currently not producing (efficiency is zero in that case)
    float inverterEfficiencyFactor = (inverterEfficiencyPercent > 0) ? inverterEfficiencyPercent/100 : 0.967;
    int32_t victronChargePower = getSolarChargePower();
    int32_t adjustedVictronChargePower = victronChargePower * inverterEfficiencyFactor;

    // Battery can be discharged and we should output max (Victron solar power || power meter value)
    if(batteryDischargeEnabled && useFullSolarPassthrough(inverter)) {
      // Case 5
      newPowerLimit = newPowerLimit > adjustedVictronChargePower ? newPowerLimit : adjustedVictronChargePower;
    } 

    // We should use Victron solar power only (corrected by efficiency factor)
    if ((solarPowerEnabled && !batteryDischargeEnabled) || (_mode == PL_MODE_SOLAR_PT_ONLY)) {
        // Case 2 - Limit power to solar power only
        MessageOutput.printf("[PowerLimiterClass::loop] Consuming Solar Power Only -> victronChargePower: %d, inverter efficiency: %.2f, powerConsumption: %d \r\n",
            victronChargePower, inverterEfficiencyFactor, newPowerLimit);

        if ((adjustedVictronChargePower < newPowerLimit) || (_mode == PL_MODE_SOLAR_PT_ONLY)) 
          newPowerLimit = adjustedVictronChargePower;
    }

    // Respect power limit
    if (newPowerLimit > config.PowerLimiter_UpperPowerLimit) 
        newPowerLimit = config.PowerLimiter_UpperPowerLimit;

    MessageOutput.printf("[PowerLimiterClass::loop] newPowerLimit: %d\r\n", newPowerLimit);
    return newPowerLimit;
}

void PowerLimiterClass::setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit)
{
    CONFIG_T& config = Configuration.get();

    // Start the inverter in case it's inactive and if the requested power is high enough
    if (!inverter->isProducing() && newPowerLimit > config.PowerLimiter_LowerPowerLimit) {
        MessageOutput.println("[PowerLimiterClass::loop] Starting up inverter...");
        inverter->sendPowerControlRequest(true);
    }

    // Stop the inverter if limit is below threshold.
    // We'll also set the power limit to the lower value in this case
    if (newPowerLimit < config.PowerLimiter_LowerPowerLimit) {
        if (inverter->isProducing()) {
            MessageOutput.println("[PowerLimiterClass::loop] Stopping inverter...");
            inverter->sendActivePowerControlRequest(static_cast<float>(config.PowerLimiter_LowerPowerLimit), PowerLimitControlType::AbsolutNonPersistent);
            _lastRequestedPowerLimit = config.PowerLimiter_LowerPowerLimit;
            inverter->sendPowerControlRequest(false);
        }
        newPowerLimit = config.PowerLimiter_LowerPowerLimit;
    }

    // Set the actual limit. We'll only do this is if the limit is in the right range
    // and differs from the last requested value
    if( _lastRequestedPowerLimit != newPowerLimit &&
          /* newPowerLimit > config.PowerLimiter_LowerPowerLimit &&  -->  This will always be true given the check above, kept for code readability */
          newPowerLimit <= config.PowerLimiter_UpperPowerLimit ) {
        MessageOutput.printf("[PowerLimiterClass::loop] Limit Non-Persistent: %d W\r\n", newPowerLimit);

        int32_t effPowerLimit = newPowerLimit;
        std::list<ChannelNum_t> dcChnls = inverter->Statistics()->getChannelsByType(TYPE_DC);
        int dcProdChnls = 0, dcTotalChnls = dcChnls.size();
        for (auto& c : dcChnls) {
            if (inverter->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC) > 1.0) {
                dcProdChnls++;
            }
        }
        if (dcProdChnls > 0) {
            effPowerLimit = round(newPowerLimit * static_cast<float>(dcTotalChnls) / dcProdChnls);
            uint16_t inverterMaxPower = inverter->DevInfo()->getMaxPower();
            if (effPowerLimit > inverterMaxPower) {
                effPowerLimit = inverterMaxPower;
            }
        }

        inverter->sendActivePowerControlRequest(static_cast<float>(effPowerLimit), PowerLimitControlType::AbsolutNonPersistent);
        _lastRequestedPowerLimit = effPowerLimit;
        // wait for the next inverter update (+ 3 seconds to make sure the limit got applied)
        _lastLimitSetTime = millis();
    }
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
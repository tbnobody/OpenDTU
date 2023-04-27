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

void PowerLimiterClass::init()
{
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

    _lastLoop = millis();

    std::shared_ptr<InverterAbstract> inverter = Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);
    if (inverter == nullptr || !inverter->isReachable()) {
        return;
    }

    // Make sure inverter is turned off if PL is disabled by user/MQTT
    // Make sure inverter is turned off when low battery threshold is reached
    if (((!config.PowerLimiter_Enabled || _disabled) && _plState != SHUTDOWN)
           || isStopThresholdReached(inverter)) {
        if (inverter->isProducing()) {
            MessageOutput.printf("PL initiated inverter shutdown.\r\n");
            inverter->sendPowerControlRequest(false);
        } else {
            _plState = SHUTDOWN;
        }
        return;
    }

    // Return if power limiter is disabled
    if (!config.PowerLimiter_Enabled || _disabled) {
      return;
    }
  	// At this point the PL is enabled but we could still be in the shutdown state 
    _plState = ACTIVE;

    // If the last inverter update is too old, don't do anything.
    // If the last inverter update was before the last limit updated, don't do anything.
    // Also give the Power meter 3 seconds time to recognize power changes after the last set limit
    // as the Hoymiles MPPT might not react immediately.
    if ((millis() - inverter->Statistics()->getLastUpdate()) > 10000
            || inverter->Statistics()->getLastUpdate() <= _lastLimitSetTime
            || PowerMeter.getLastPowerMeterUpdate() <= (_lastLimitSetTime + 3000)) {
        return;
    }

    // Printout some stats
    if (millis() - PowerMeter.getLastPowerMeterUpdate() < (30 * 1000)) {
        float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);
        MessageOutput.printf("[PowerLimiterClass::loop] dcVoltage: %.2f Voltage Start Threshold: %.2f Voltage Stop Threshold: %.2f inverter->isProducing(): %d\r\n",
            dcVoltage, config.PowerLimiter_VoltageStartThreshold, config.PowerLimiter_VoltageStopThreshold, inverter->isProducing());
    }


    // Battery charging cycle conditions
    // The battery can only be discharged after a full charge in the
    // EMPTY_WHEN_FULL case 
    if (isStopThresholdReached(inverter)) {
      // Disable battery discharge when empty
      _batteryDischargeEnabled = false;
    } else if (config.PowerLimiter_BatteryDrainStategy == EMPTY_AT_NIGHT) {
      // Enable battery discharge when there is no sunshine
      _batteryDischargeEnabled = !canUseDirectSolarPower();
    }

    // This checks if the battery discharge start conditions are met for the EMPTY_WHEN_FULL case
    if (isStartThresholdReached(inverter) && config.PowerLimiter_BatteryDrainStategy == EMPTY_WHEN_FULL) {
      _batteryDischargeEnabled = true;
    }

    // Calculate and set Power Limit
    int32_t newPowerLimit = calcPowerLimit(inverter, canUseDirectSolarPower(), _batteryDischargeEnabled);
    setNewPowerLimit(inverter, newPowerLimit);
}

plStates PowerLimiterClass::getPowerLimiterState() {
    return _plState;
}

int32_t PowerLimiterClass::getLastRequestedPowewrLimit() {
    return _lastRequestedPowerLimit;
}

bool PowerLimiterClass::getDisable() {
    return _disabled;
}

void PowerLimiterClass::setDisable(bool disable) {
    _disabled = disable;
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




int32_t PowerLimiterClass::calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled)
{
    CONFIG_T& config = Configuration.get();
    
    int32_t newPowerLimit = round(PowerMeter.getPowerTotal());

    if (!solarPowerEnabled && !batteryDischargeEnabled) {
      // No energy sources available
      return 0;
    }

    // Safety check, return on too old power meter values
    if (millis() - PowerMeter.getLastPowerMeterUpdate() > (30 * 1000)
            && (millis() - inverter->Statistics()->getLastUpdate()) > (config.Dtu_PollInterval * 10 * 1000)) {
        // If the power meter values are older than 30 seconds, 
        // and the Inverter Stats are older then 10x the poll interval
        // set the limit to 0W for safety reasons.
        MessageOutput.println("[PowerLimiterClass::loop] Power Meter/Inverter values too old. Using 0W (i.e. disable inverter)");
        return 0;
    }

    if (config.PowerLimiter_IsInverterBehindPowerMeter) {
        // If the inverter the behind the power meter (part of measurement),
        // the produced power of this inverter has also to be taken into account.
        // We don't use FLD_PAC from the statistics, because that
        // data might be too old and unreliable.
        float acPower = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_PAC); 
        newPowerLimit += static_cast<int>(acPower);
    }

    // We're not trying to hit 0 exactly but take an offset into account
    // This means we never fully compensate the used power with the inverter 
    newPowerLimit -= config.PowerLimiter_TargetPowerConsumption;

    // Check if the new value is within the limits of the hysteresis and
    // if we can discharge the battery
    // If things did not change much we just use the old setting
    if (newPowerLimit >= (-config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
        newPowerLimit <= (+config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
        batteryDischargeEnabled ) {
            MessageOutput.println("[PowerLimiterClass::loop] reusing old limit");
            return _lastRequestedPowerLimit;
    }

    // We should use Victron solar power only (corrected by efficiency factor)
    if (solarPowerEnabled && !batteryDischargeEnabled) {
        float efficiency = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_EFF);
        int32_t victronChargePower = this->getDirectSolarPower();
        int32_t adjustedVictronChargePower = victronChargePower * (efficiency > 0.0 ? (efficiency / 100.0) : 1.0); // if inverter is off, use 1.0

        MessageOutput.printf("[PowerLimiterClass::loop] Consuming Solar Power Only -> victronChargePower: %d, efficiency: %.2f, powerConsumption: %d \r\n", 
            victronChargePower, efficiency, newPowerLimit);

        // Limit power to solar power only
        if (adjustedVictronChargePower < newPowerLimit)
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

        inverter->sendActivePowerControlRequest(effPowerLimit, PowerLimitControlType::AbsolutNonPersistent);
        _lastRequestedPowerLimit = newPowerLimit;
        // wait for the next inverter update (+ 3 seconds to make sure the limit got applied)
        _lastLimitSetTime = millis();
    }
}

int32_t PowerLimiterClass::getDirectSolarPower()
{
    if (!canUseDirectSolarPower()) {
        return 0;
    }

    return VeDirect.veFrame.PPV;
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
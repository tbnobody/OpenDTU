// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */

#include "Battery.h"
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
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;


    CONFIG_T& config = Configuration.get();

    // Zero export power limiter
    if (strlen(config.PowerLimiter_MqttTopicPowerMeter1) != 0) {
        MqttSettings.subscribe(config.PowerLimiter_MqttTopicPowerMeter1, 0, std::bind(&PowerLimiterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    }

    if (strlen(config.PowerLimiter_MqttTopicPowerMeter2) != 0) {
        MqttSettings.subscribe(config.PowerLimiter_MqttTopicPowerMeter2, 0, std::bind(&PowerLimiterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    }

    if (strlen(config.PowerLimiter_MqttTopicPowerMeter3) != 0) {
        MqttSettings.subscribe(config.PowerLimiter_MqttTopicPowerMeter3, 0, std::bind(&PowerLimiterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    }

     _lastCommandSent = 0;
    _lastLoop = 0;
    _lastPowerMeterUpdate = 0;
    _lastRequestedPowerLimit = 0;
}

void PowerLimiterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    CONFIG_T& config = Configuration.get();

    if (strcmp(topic, config.PowerLimiter_MqttTopicPowerMeter1) == 0) {
        _powerMeter1Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
    }

    if (strcmp(topic, config.PowerLimiter_MqttTopicPowerMeter2) == 0) {
        _powerMeter2Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
    }

    if (strcmp(topic, config.PowerLimiter_MqttTopicPowerMeter3) == 0) {
        _powerMeter3Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
    }

    _lastPowerMeterUpdate = millis();
}

void PowerLimiterClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter_Enabled
            || !MqttSettings.getConnected()
            || !Hoymiles.getRadio()->isIdle()
            || (millis() - _lastCommandSent) < (config.PowerLimiter_Interval * 1000)
            || (millis() - _lastLoop) < (config.PowerLimiter_Interval * 1000)) {
        if (!config.PowerLimiter_Enabled)
            _plState = STATE_DISCOVER; // ensure STATE_DISCOVER is set, if PowerLimiter will be enabled.
        return;
    }

    _lastLoop = millis();

    std::shared_ptr<InverterAbstract> inverter = Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);
    if (inverter == nullptr || !inverter->isReachable()) {
        return;
    }

    float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);
    float acPower = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_PAC); 
    float correctedDcVoltage = dcVoltage + (acPower * config.PowerLimiter_VoltageLoadCorrectionFactor);

    if ((millis() - inverter->Statistics()->getLastUpdate()) > 10000) {
        return;
    }

    if (millis() - _lastPowerMeterUpdate < (30 * 1000)) {
        MessageOutput.printf("[PowerLimiterClass::loop] dcVoltage: %.2f Voltage Start Threshold: %.2f Voltage Stop Threshold: %.2f inverter->isProducing(): %d\r\n",
            dcVoltage, config.PowerLimiter_VoltageStartThreshold, config.PowerLimiter_VoltageStopThreshold, inverter->isProducing());
    }


    while(true) {
        switch(_plState) {
            case STATE_DISCOVER:
                if (!inverter->isProducing() || isStopThresholdReached(inverter)) {
                    _plState = STATE_OFF;
                }
                else if (canUseDirectSolarPower()) {
                    _plState = STATE_CONSUME_SOLAR_POWER_ONLY;
                }
                else {
                    _plState = STATE_NORMAL_OPERATION;
                }
                break;
            case STATE_OFF:
                // if on turn off
                if (inverter->isProducing()) {
                    MessageOutput.printf("[PowerLimiterClass::loop] DC voltage: %.2f Corrected DC voltage: %.2f...\r\n",
                        dcVoltage, correctedDcVoltage);
                    MessageOutput.println("[PowerLimiterClass::loop] Stopping inverter...");
                    inverter->sendPowerControlRequest(Hoymiles.getRadio(), false);
                    setNewPowerLimit(inverter, config.PowerLimiter_LowerPowerLimit);
                    return;
                }

                // do nothing if battery is empty
                if (isStopThresholdReached(inverter)) 
                    return;
                // check for possible state changes
                if (isStartThresholdReached(inverter) && calcPowerLimit(inverter, false) >= config.PowerLimiter_LowerPowerLimit) {
                    _plState = STATE_NORMAL_OPERATION;
                }
                else if (canUseDirectSolarPower() && calcPowerLimit(inverter, true) >= config.PowerLimiter_LowerPowerLimit) {
                    _plState = STATE_CONSUME_SOLAR_POWER_ONLY;
                }

                // inverter on on state change
                if (_plState != STATE_OFF) {
                    // DC voltage high enough, start the inverter
                    MessageOutput.println("[PowerLimiterClass::loop] Starting up inverter...");
                    inverter->sendPowerControlRequest(Hoymiles.getRadio(), true);
                    _lastCommandSent = millis();
                    return;
                }
                else
                    return;
                break;
            case STATE_CONSUME_SOLAR_POWER_ONLY: {
                int32_t newPowerLimit = calcPowerLimit(inverter, true);
                if (!inverter->isProducing() 
                        || isStopThresholdReached(inverter)
                        || (newPowerLimit < config.PowerLimiter_LowerPowerLimit && config.PowerLimiter_BatteryDrainStategy == EMPTY_WHEN_FULL)) {
                    _plState = STATE_OFF;
                    break;
                }
                else if (!canUseDirectSolarPower() || isStartThresholdReached(inverter)) {
                    _plState = STATE_NORMAL_OPERATION;
                    break;
                }
                setNewPowerLimit(inverter, newPowerLimit);
                return;
                break;
            }
            case STATE_NORMAL_OPERATION: {
                int32_t newPowerLimit = calcPowerLimit(inverter, false);
                if (!inverter->isProducing() 
                        || isStopThresholdReached(inverter)
                        || newPowerLimit < config.PowerLimiter_LowerPowerLimit) {
                    _plState = STATE_OFF;
                    break;
                }
                // check if grid power consumption is within the upper an lower threshold of the target consumption
                else if (newPowerLimit >= (config.PowerLimiter_TargetPowerConsumption - config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
                    newPowerLimit <= (config.PowerLimiter_TargetPowerConsumption + config.PowerLimiter_TargetPowerConsumptionHysteresis)) {
                        return;
                }
                setNewPowerLimit(inverter, newPowerLimit);
                return;
                break;
            } 
        }
    }
}

plStates PowerLimiterClass::getPowerLimiterState() {
    return _plState;
}

int32_t PowerLimiterClass::getLastRequestedPowewrLimit() {
    return _lastRequestedPowerLimit;
}

bool PowerLimiterClass::canUseDirectSolarPower()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter_SolarPassTroughEnabled
            || !config.Vedirect_Enabled) {
        return false;
    }

    if (VeDirect.veFrame.PPV < 10) {
        // Not enough power
        return false;
    }

    return true;
}

int32_t PowerLimiterClass::calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool consumeSolarPowerOnly)
{
    CONFIG_T& config = Configuration.get();
    
    int32_t newPowerLimit = round(_powerMeter1Power + _powerMeter2Power + _powerMeter3Power);

    float efficency = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_EFF);
    int32_t victronChargePower = this->getDirectSolarPower();
    int32_t adjustedVictronChargePower = victronChargePower * (efficency > 0.0 ? (efficency / 100.0) : 1.0); // if inverter is off, use 1.0

    MessageOutput.printf("[PowerLimiterClass::loop] victronChargePower: %d, efficiency: %.2f, consumeSolarPowerOnly: %s, powerConsumption: %d \r\n", 
        victronChargePower, efficency, consumeSolarPowerOnly ? "true" : "false", newPowerLimit);
   
    if (millis() - _lastPowerMeterUpdate < (30 * 1000)) {
        if (config.PowerLimiter_IsInverterBehindPowerMeter) {
            // If the inverter the behind the power meter (part of measurement),
            // the produced power of this inverter has also to be taken into account.
            // We don't use FLD_PAC from the statistics, because that
            // data might be too old and unrelieable.
            newPowerLimit += _lastRequestedPowerLimit;
        }

        newPowerLimit -= config.PowerLimiter_TargetPowerConsumption;

        int32_t upperPowerLimit = config.PowerLimiter_UpperPowerLimit;
        if (consumeSolarPowerOnly && (upperPowerLimit > adjustedVictronChargePower)) {
            // Battery voltage too low, use Victron solar power (corrected by efficency factor) only
            upperPowerLimit = adjustedVictronChargePower;
        }

        if (newPowerLimit > upperPowerLimit) 
            newPowerLimit = upperPowerLimit;
    } else {
        // If the power meter values are older than 30 seconds,
        // set the limit to config.PowerLimiter_LowerPowerLimit for safety reasons.
        newPowerLimit = config.PowerLimiter_LowerPowerLimit;
    }
    return newPowerLimit;
}

void PowerLimiterClass::setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit)
{
    MessageOutput.printf("[PowerLimiterClass::loop] Limit Non-Persistent: %d W\r\n", newPowerLimit);
    inverter->sendActivePowerControlRequest(Hoymiles.getRadio(), newPowerLimit, PowerLimitControlType::AbsolutNonPersistent);
    _lastRequestedPowerLimit = newPowerLimit;
    _lastCommandSent = millis();
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

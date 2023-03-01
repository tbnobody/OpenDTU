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

    _consumeSolarPowerOnly = true;
    _lastCommandSent = 0;
    _lastLoop = 0;
    _lastPowerMeterUpdate = 0;
    _lastRequestedPowerLimit = 0;
}

void PowerLimiterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    MessageOutput.printf("PowerLimiterClass: Received MQTT message on topic: %s\r\n", topic);

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
        return;
    }

    _lastLoop = millis();

    std::shared_ptr<InverterAbstract> inverter = Hoymiles.getInverterByPos(config.PowerLimiter_InverterId);

    if (inverter == nullptr || !inverter->isReachable()) {
        return;
    }

    float dcVoltage = inverter->Statistics()->getChannelFieldValue(TYPE_DC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_UDC);

    if ((millis() - inverter->Statistics()->getLastUpdate()) > 10000) {
        return;
    }

    float efficency = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_EFF);
    uint32_t victronChargePower = this->getDirectSolarPower();

    MessageOutput.printf("[PowerLimiterClass::loop] victronChargePower: %d, efficiency: %.2f, consumeSolarPowerOnly: %s \r\n", victronChargePower, efficency, _consumeSolarPowerOnly ? "true" : "false");
   
    if (millis() - _lastPowerMeterUpdate < (30 * 1000)) {
        MessageOutput.printf("[PowerLimiterClass::loop] dcVoltage: %.2f Voltage Start Threshold: %.2f Voltage Stop Threshold: %.2f inverter->isProducing(): %d\r\n",
            dcVoltage, config.PowerLimiter_VoltageStartThreshold, config.PowerLimiter_VoltageStopThreshold, inverter->isProducing());
    }

    int32_t powerMeter = _powerMeter1Power + _powerMeter2Power + _powerMeter3Power;

    if (inverter->isProducing()) {
        float acPower = inverter->Statistics()->getChannelFieldValue(TYPE_AC, (ChannelNum_t) config.PowerLimiter_InverterChannelId, FLD_PAC); 
        float correctedDcVoltage = dcVoltage + (acPower * config.PowerLimiter_VoltageLoadCorrectionFactor);

        if ((_consumeSolarPowerOnly && isStartThresholdReached(inverter))) {
            // The battery is full enough again, use the full battery power from now on.
            _consumeSolarPowerOnly = false;
        } else if (!_consumeSolarPowerOnly && !isStopThresholdReached(inverter) && canUseDirectSolarPower()) {
            // The battery voltage dropped too low
            _consumeSolarPowerOnly = true;
        }

        if (isStopThresholdReached(inverter)
                || (_consumeSolarPowerOnly && !canUseDirectSolarPower())) {
            // DC voltage too low, stop the inverter
            MessageOutput.printf("[PowerLimiterClass::loop] DC voltage: %.2f Corrected DC voltage: %.2f...\r\n",
                dcVoltage, correctedDcVoltage);
            MessageOutput.println("[PowerLimiterClass::loop] Stopping inverter...");
            inverter->sendPowerControlRequest(Hoymiles.getRadio(), false);

            uint16_t newPowerLimit = (uint16_t)config.PowerLimiter_LowerPowerLimit;
            inverter->sendActivePowerControlRequest(Hoymiles.getRadio(), newPowerLimit, PowerLimitControlType::AbsolutNonPersistent);
            _lastRequestedPowerLimit = newPowerLimit;

            _lastCommandSent = millis();
            _consumeSolarPowerOnly = false;

            return;
        }
    } else {
        if ((isStartThresholdReached(inverter) || (canUseDirectSolarPower() && (!isStopThresholdReached(inverter)))) 
                && powerMeter >= config.PowerLimiter_LowerPowerLimit) {
            // DC voltage high enough, start the inverter
            MessageOutput.println("[PowerLimiterClass::loop] Starting up inverter...");
            _lastCommandSent = millis();
            inverter->sendPowerControlRequest(Hoymiles.getRadio(), true);

            // In this mode, the inverter should consume the current solar power only
            // and not drain additional power from the battery
            if (!isStartThresholdReached(inverter)) {
                _consumeSolarPowerOnly = true;
            }
        }

        return;
    }

    int32_t newPowerLimit = 0;

    if (millis() - _lastPowerMeterUpdate < (30 * 1000)) {
        newPowerLimit = powerMeter;
        // check if grid power consumption is within the upper an lower threshold of the target consumption
        if (!_consumeSolarPowerOnly &&
            newPowerLimit >= (config.PowerLimiter_TargetPowerConsumption - config.PowerLimiter_TargetPowerConsumptionHysteresis) &&
            newPowerLimit <= (config.PowerLimiter_TargetPowerConsumption + config.PowerLimiter_TargetPowerConsumptionHysteresis))
            return;
        else {
            if (config.PowerLimiter_IsInverterBehindPowerMeter) {
                // If the inverter the behind the power meter (part of measurement),
                // the produced power of this inverter has also to be taken into account.
                // We don't use FLD_PAC from the statistics, because that
                // data might be too old and unrelieable.
                newPowerLimit += _lastRequestedPowerLimit;
            }

            newPowerLimit -= config.PowerLimiter_TargetPowerConsumption;

            uint16_t upperPowerLimit = config.PowerLimiter_UpperPowerLimit;
            if (_consumeSolarPowerOnly && (upperPowerLimit > victronChargePower)) {
                // Battery voltage too low, use Victron solar power (corrected by efficency factor) only
                upperPowerLimit = victronChargePower * (efficency / 100.0);
            }

            if (newPowerLimit > upperPowerLimit) 
                newPowerLimit = upperPowerLimit;
            else if (newPowerLimit < (uint16_t)config.PowerLimiter_LowerPowerLimit) {
                newPowerLimit = (uint16_t)config.PowerLimiter_LowerPowerLimit;
                // stop the inverter
                MessageOutput.println("[PowerLimiterClass::loop] Power limit below lower power limit. Stopping inverter...");
                inverter->sendPowerControlRequest(Hoymiles.getRadio(), false);

            }

            MessageOutput.printf("[PowerLimiterClass::loop] powerMeter: %d W lastRequestedPowerLimit: %d\r\n",
                powerMeter, _lastRequestedPowerLimit);
        }
    } else {
        // If the power meter values are older than 30 seconds,
        // set the limit to config.PowerLimiter_LowerPowerLimit for safety reasons.
        newPowerLimit = config.PowerLimiter_LowerPowerLimit;
    }

    MessageOutput.printf("[PowerLimiterClass::loop] Limit Non-Persistent: %d W\r\n", newPowerLimit);
    inverter->sendActivePowerControlRequest(Hoymiles.getRadio(), newPowerLimit, PowerLimitControlType::AbsolutNonPersistent);
    _lastRequestedPowerLimit = newPowerLimit;

    _lastCommandSent = millis();
}

bool PowerLimiterClass::canUseDirectSolarPower()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter_SolarPassTroughEnabled
            || !config.Vedirect_Enabled) {
        return false;
    }

    if (VeDirect.veFrame.PPV < 10.0) {
        // Not enough power
        return false;
    }

    return true;
}

uint16_t PowerLimiterClass::getDirectSolarPower()
{
    if (!canUseDirectSolarPower()) {
        return 0;
    }

    return round(VeDirect.veFrame.PPV);
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

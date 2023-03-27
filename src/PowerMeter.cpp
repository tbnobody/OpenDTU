// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PowerMeter.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "SDM.h"
#include "MessageOutput.h"
#include <ctime>

PowerMeterClass PowerMeter;

SDM sdm(Serial2, 9600, NOT_A_PIN, SERIAL_8N1, SDM_RX_PIN, SDM_TX_PIN);

void PowerMeterClass::init()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _lastPowerMeterUpdate = 0;

    CONFIG_T& config = Configuration.get();

    MqttSettings.subscribe(config.PowerMeter_MqttTopicPowerMeter1, 0, std::bind(&PowerMeterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(config.PowerMeter_MqttTopicPowerMeter2, 0, std::bind(&PowerMeterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(config.PowerMeter_MqttTopicPowerMeter3, 0, std::bind(&PowerMeterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
       
    mqttInitDone = true;

    sdm.begin();
}

void PowerMeterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    CONFIG_T& config = Configuration.get();
    if(config.PowerMeter_Enabled && config.PowerMeter_Source == 0){

        if (strcmp(topic, config.PowerMeter_MqttTopicPowerMeter1) == 0) {
            _powerMeter1Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
        }

        if (strcmp(topic, config.PowerMeter_MqttTopicPowerMeter2) == 0) {
            _powerMeter2Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
        }

        if (strcmp(topic, config.PowerMeter_MqttTopicPowerMeter3) == 0) {
            _powerMeter3Power = std::stof(std::string(reinterpret_cast<const char*>(payload), (unsigned int)len));
        }
        
        MessageOutput.printf("PowerMeterClass: TotalPower: %5.2f\n", getPowerTotal());
    }

    _lastPowerMeterUpdate = millis();
}

float PowerMeterClass::getPowerTotal(){
    return _powerMeter1Power + _powerMeter2Power + _powerMeter3Power;
}

uint32_t PowerMeterClass::getLastPowerMeterUpdate(){
    return _lastPowerMeterUpdate;
}

void PowerMeterClass::mqtt(){
    if (!MqttSettings.getConnected()){
        return;
    }else{
        String topic = "powermeter";
        MqttSettings.publish(topic + "/power1", String(_powerMeter1Power));
        MqttSettings.publish(topic + "/power2", String(_powerMeter2Power));
        MqttSettings.publish(topic + "/power3", String(_powerMeter3Power));
        MqttSettings.publish(topic + "/powertotal", String(getPowerTotal()));
        MqttSettings.publish(topic + "/voltage1", String(_powerMeter1Voltage));
        MqttSettings.publish(topic + "/voltage2", String(_powerMeter2Voltage));
        MqttSettings.publish(topic + "/voltage3", String(_powerMeter3Voltage));
        MqttSettings.publish(topic + "/import", String(_PowerMeterImport));
        MqttSettings.publish(topic + "/export", String(_PowerMeterExport));
    }
}

void PowerMeterClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if(config.PowerMeter_Enabled && millis() - _lastPowerMeterUpdate >= (config.PowerMeter_Interval * 1000)){
        uint8_t _address = config.PowerMeter_SdmAddress;
        if(config.PowerMeter_Source == 1){
            _powerMeter1Power = static_cast<float>(sdm.readVal(SDM_PHASE_1_POWER, _address));
            _powerMeter2Power = 0.0;
            _powerMeter3Power = 0.0;
            _powerMeter1Voltage = static_cast<float>(sdm.readVal(SDM_PHASE_1_VOLTAGE, _address));
            _powerMeter2Voltage = 0.0;
            _powerMeter3Voltage = 0.0;
            _PowerMeterImport = static_cast<float>(sdm.readVal(SDM_IMPORT_ACTIVE_ENERGY, _address));
            _PowerMeterExport = static_cast<float>(sdm.readVal(SDM_EXPORT_ACTIVE_ENERGY, _address));
        }
        if(config.PowerMeter_Source == 2){
            _powerMeter1Power = static_cast<float>(sdm.readVal(SDM_PHASE_1_POWER, _address));
            _powerMeter2Power = static_cast<float>(sdm.readVal(SDM_PHASE_2_POWER, _address));
            _powerMeter3Power = static_cast<float>(sdm.readVal(SDM_PHASE_3_POWER, _address));
            _powerMeter1Voltage = static_cast<float>(sdm.readVal(SDM_PHASE_1_VOLTAGE, _address));
            _powerMeter2Voltage = static_cast<float>(sdm.readVal(SDM_PHASE_2_VOLTAGE, _address));
            _powerMeter3Voltage = static_cast<float>(sdm.readVal(SDM_PHASE_3_VOLTAGE, _address));
            _PowerMeterImport = static_cast<float>(sdm.readVal(SDM_IMPORT_ACTIVE_ENERGY, _address));
            _PowerMeterExport = static_cast<float>(sdm.readVal(SDM_EXPORT_ACTIVE_ENERGY, _address));
        }
        
        MessageOutput.printf("PowerMeterClass: TotalPower: %5.2f\n", getPowerTotal());
        
        mqtt();

        _lastPowerMeterUpdate = millis();
    }
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PowerMeter.h"
#include "Configuration.h"
#include "HttpPowerMeter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "SDM.h"
#include "MessageOutput.h"
#include <ctime>
#include <SoftwareSerial.h>

PowerMeterClass PowerMeter;

SDM sdm(Serial2, 9600, NOT_A_PIN, SERIAL_8N1, SDM_RX_PIN, SDM_TX_PIN);

SoftwareSerial inputSerial;

void PowerMeterClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&PowerMeterClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    _lastPowerMeterCheck = 0;
    _lastPowerMeterUpdate = 0;

    for (auto const& s: _mqttSubscriptions) { MqttSettings.unsubscribe(s.first); }
    _mqttSubscriptions.clear();

    CONFIG_T& config = Configuration.get();

    if (!config.PowerMeter.Enabled) {
        return;
    }

    switch(config.PowerMeter.Source) {
    case SOURCE_MQTT: {
        auto subscribe = [this](char const* topic, float* target) {
            if (strlen(topic) == 0) { return; }
            MqttSettings.subscribe(topic, 0,
                    std::bind(&PowerMeterClass::onMqttMessage,
                        this, std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4,
                        std::placeholders::_5, std::placeholders::_6)
                    );
            _mqttSubscriptions.try_emplace(topic, target);
        };

        subscribe(config.PowerMeter.MqttTopicPowerMeter1, &_powerMeter1Power);
        subscribe(config.PowerMeter.MqttTopicPowerMeter2, &_powerMeter2Power);
        subscribe(config.PowerMeter.MqttTopicPowerMeter3, &_powerMeter3Power);
        break;
    }

    case SOURCE_SDM1PH:
    case SOURCE_SDM3PH:
        sdm.begin();
        break;

    case SOURCE_HTTP:
        HttpPowerMeter.init();
        break;

    case SOURCE_SML:
        pinMode(SML_RX_PIN, INPUT);
        inputSerial.begin(9600, SWSERIAL_8N1, SML_RX_PIN, -1, false, 128, 95);
        inputSerial.enableRx(true);
        inputSerial.enableTx(false);
        inputSerial.flush();
        break;
    }
}

void PowerMeterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    for (auto const& subscription: _mqttSubscriptions) {
        if (subscription.first != topic) { continue; }

        std::string value(reinterpret_cast<const char*>(payload), len);
        try {
            *subscription.second = std::stof(value);
        }
        catch(std::invalid_argument const& e) {
            MessageOutput.printf("PowerMeterClass: cannot parse payload of topic '%s' as float: %s\r\n",
                    topic, value.c_str());
            return;
        }

        if (_verboseLogging) {
            MessageOutput.printf("PowerMeterClass: Updated from '%s', TotalPower: %5.2f\r\n",
                    topic, getPowerTotal());
        }

        _lastPowerMeterUpdate = millis();
    }
}

float PowerMeterClass::getPowerTotal(bool forceUpdate)
{
    if (forceUpdate) {
        CONFIG_T& config = Configuration.get();
        if (config.PowerMeter.Enabled
                && (millis() - _lastPowerMeterUpdate) > (1000)) {
            readPowerMeter();
        }
    }

    std::lock_guard<std::mutex> l(_mutex);
    return _powerMeter1Power + _powerMeter2Power + _powerMeter3Power;
}

uint32_t PowerMeterClass::getLastPowerMeterUpdate()
{
    std::lock_guard<std::mutex> l(_mutex);
    return _lastPowerMeterUpdate;
}

void PowerMeterClass::mqtt()
{
    if (!MqttSettings.getConnected()) { return; }

    String topic = "powermeter";
    auto totalPower = getPowerTotal();

    std::lock_guard<std::mutex> l(_mutex);
    MqttSettings.publish(topic + "/power1", String(_powerMeter1Power));
    MqttSettings.publish(topic + "/power2", String(_powerMeter2Power));
    MqttSettings.publish(topic + "/power3", String(_powerMeter3Power));
    MqttSettings.publish(topic + "/powertotal", String(totalPower));
    MqttSettings.publish(topic + "/voltage1", String(_powerMeter1Voltage));
    MqttSettings.publish(topic + "/voltage2", String(_powerMeter2Voltage));
    MqttSettings.publish(topic + "/voltage3", String(_powerMeter3Voltage));
    MqttSettings.publish(topic + "/import", String(_powerMeterImport));
    MqttSettings.publish(topic + "/export", String(_powerMeterExport));
}

void PowerMeterClass::loop()
{
    CONFIG_T const& config = Configuration.get();
    _verboseLogging = config.PowerMeter.VerboseLogging;

    if (!config.PowerMeter.Enabled) { return; }

    if (config.PowerMeter.Source == SOURCE_SML) {
        if (!smlReadLoop()) {
            return;
        } else {
            _lastPowerMeterUpdate = millis();
        }
    }

    if ((millis() - _lastPowerMeterCheck) < (config.PowerMeter.Interval * 1000)) {
        return;
    }

    readPowerMeter();

    MessageOutput.printf("PowerMeterClass: TotalPower: %5.2f\r\n", getPowerTotal());

    mqtt();

    _lastPowerMeterCheck = millis();
}

void PowerMeterClass::readPowerMeter()
{
    CONFIG_T& config = Configuration.get();
    
    uint8_t _address = config.PowerMeter.SdmAddress;

    if (config.PowerMeter.Source == SOURCE_SDM1PH) {
        // this takes a "very long" time as each readVal() is a synchronous
        // exchange of serial messages. cache the values and write later.
        auto phase1Power = sdm.readVal(SDM_PHASE_1_POWER, _address);
        auto phase1Voltage = sdm.readVal(SDM_PHASE_1_VOLTAGE, _address);
        auto energyImport = sdm.readVal(SDM_IMPORT_ACTIVE_ENERGY, _address);
        auto energyExport = sdm.readVal(SDM_EXPORT_ACTIVE_ENERGY, _address);

        std::lock_guard<std::mutex> l(_mutex);
        _powerMeter1Power = static_cast<float>(phase1Power);
        _powerMeter2Power = 0;
        _powerMeter3Power = 0;
        _powerMeter1Voltage = static_cast<float>(phase1Voltage);
        _powerMeter2Voltage = 0;
        _powerMeter3Voltage = 0;
        _powerMeterImport = static_cast<float>(energyImport);
        _powerMeterExport = static_cast<float>(energyExport);
        _lastPowerMeterUpdate = millis();
    }
    else if (config.PowerMeter.Source == SOURCE_SDM3PH) {
        // this takes a "very long" time as each readVal() is a synchronous
        // exchange of serial messages. cache the values and write later.
        auto phase1Power = sdm.readVal(SDM_PHASE_1_POWER, _address);
        auto phase2Power = sdm.readVal(SDM_PHASE_2_POWER, _address);
        auto phase3Power = sdm.readVal(SDM_PHASE_3_POWER, _address);
        auto phase1Voltage = sdm.readVal(SDM_PHASE_1_VOLTAGE, _address);
        auto phase2Voltage = sdm.readVal(SDM_PHASE_2_VOLTAGE, _address);
        auto phase3Voltage = sdm.readVal(SDM_PHASE_3_VOLTAGE, _address);
        auto energyImport = sdm.readVal(SDM_IMPORT_ACTIVE_ENERGY, _address);
        auto energyExport = sdm.readVal(SDM_EXPORT_ACTIVE_ENERGY, _address);

        std::lock_guard<std::mutex> l(_mutex);
        _powerMeter1Power = static_cast<float>(phase1Power);
        _powerMeter2Power = static_cast<float>(phase2Power);
        _powerMeter3Power = static_cast<float>(phase3Power);
        _powerMeter1Voltage = static_cast<float>(phase1Voltage);
        _powerMeter2Voltage = static_cast<float>(phase2Voltage);
        _powerMeter3Voltage = static_cast<float>(phase3Voltage);
        _powerMeterImport = static_cast<float>(energyImport);
        _powerMeterExport = static_cast<float>(energyExport);
        _lastPowerMeterUpdate = millis();
    }
    else if (config.PowerMeter.Source == SOURCE_HTTP) {
        if (HttpPowerMeter.updateValues()) {
            std::lock_guard<std::mutex> l(_mutex);
            _powerMeter1Power = HttpPowerMeter.getPower(1);
            _powerMeter2Power = HttpPowerMeter.getPower(2);
            _powerMeter3Power = HttpPowerMeter.getPower(3);
            _lastPowerMeterUpdate = millis();
        }
    }
}

bool PowerMeterClass::smlReadLoop()
{
    while (inputSerial.available()) {
        double readVal = 0;
        unsigned char smlCurrentChar = inputSerial.read();
        sml_states_t smlCurrentState = smlState(smlCurrentChar);
        if (smlCurrentState == SML_LISTEND) {
            for (auto& handler: smlHandlerList) {
                if (smlOBISCheck(handler.OBIS)) {
                    handler.Fn(readVal);
                    *handler.Arg = readVal;
                }
            }
        } else if (smlCurrentState == SML_FINAL) {
            return true;
        }
    }

    return false;
}

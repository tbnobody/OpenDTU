// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PowerMeter.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "HttpPowerMeter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "MessageOutput.h"
#include <ctime>
#include <SMA_HM.h>

PowerMeterClass PowerMeter;

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

    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.printf("[PowerMeter] rx = %d, tx = %d, dere = %d\r\n",
            pin.powermeter_rx, pin.powermeter_tx, pin.powermeter_dere);

    switch(static_cast<Source>(config.PowerMeter.Source)) {
    case Source::MQTT: {
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

    case Source::SDM1PH:
    case Source::SDM3PH:
        if (pin.powermeter_rx < 0 || pin.powermeter_tx < 0) {
            MessageOutput.println("[PowerMeter] invalid pin config for SDM power meter (RX and TX pins must be defined)");
            return;
        }

        _upSdm = std::make_unique<SDM>(Serial2, 9600, pin.powermeter_dere,
                SERIAL_8N1, pin.powermeter_rx, pin.powermeter_tx);
        _upSdm->begin();
        break;

    case Source::HTTP:
        HttpPowerMeter.init();
        break;

    case Source::SML:
        if (pin.powermeter_rx < 0) {
            MessageOutput.println("[PowerMeter] invalid pin config for SML power meter (RX pin must be defined)");
            return;
        }

        pinMode(pin.powermeter_rx, INPUT);
        _upSmlSerial = std::make_unique<SoftwareSerial>();
        _upSmlSerial->begin(9600, SWSERIAL_8N1, pin.powermeter_rx, -1, false, 128, 95);
        _upSmlSerial->enableRx(true);
        _upSmlSerial->enableTx(false);
        _upSmlSerial->flush();
        break;

    case Source::SMAHM2:
        SMA_HM.init(scheduler, config.PowerMeter.VerboseLogging);
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

bool PowerMeterClass::isDataValid()
{
    auto const& config = Configuration.get();

    std::lock_guard<std::mutex> l(_mutex);

    bool valid = config.PowerMeter.Enabled &&
        _lastPowerMeterUpdate > 0 &&
        ((millis() - _lastPowerMeterUpdate) < (30 * 1000));

    // reset if timed out to avoid glitch once
    // (millis() - _lastPowerMeterUpdate) overflows
    if (!valid) { _lastPowerMeterUpdate = 0; }

    return valid;
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

    if (static_cast<Source>(config.PowerMeter.Source) == Source::SML &&
            nullptr != _upSmlSerial) {
        if (!smlReadLoop()) { return; }
        _lastPowerMeterUpdate = millis();
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
    Source configuredSource = static_cast<Source>(config.PowerMeter.Source);

    if (configuredSource == Source::SDM1PH) {
        if (!_upSdm) { return; }

        // this takes a "very long" time as each readVal() is a synchronous
        // exchange of serial messages. cache the values and write later.
        auto phase1Power = _upSdm->readVal(SDM_PHASE_1_POWER, _address);
        auto phase1Voltage = _upSdm->readVal(SDM_PHASE_1_VOLTAGE, _address);
        auto energyImport = _upSdm->readVal(SDM_IMPORT_ACTIVE_ENERGY, _address);
        auto energyExport = _upSdm->readVal(SDM_EXPORT_ACTIVE_ENERGY, _address);

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
    else if (configuredSource == Source::SDM3PH) {
        if (!_upSdm) { return; }

        // this takes a "very long" time as each readVal() is a synchronous
        // exchange of serial messages. cache the values and write later.
        auto phase1Power = _upSdm->readVal(SDM_PHASE_1_POWER, _address);
        auto phase2Power = _upSdm->readVal(SDM_PHASE_2_POWER, _address);
        auto phase3Power = _upSdm->readVal(SDM_PHASE_3_POWER, _address);
        auto phase1Voltage = _upSdm->readVal(SDM_PHASE_1_VOLTAGE, _address);
        auto phase2Voltage = _upSdm->readVal(SDM_PHASE_2_VOLTAGE, _address);
        auto phase3Voltage = _upSdm->readVal(SDM_PHASE_3_VOLTAGE, _address);
        auto energyImport = _upSdm->readVal(SDM_IMPORT_ACTIVE_ENERGY, _address);
        auto energyExport = _upSdm->readVal(SDM_EXPORT_ACTIVE_ENERGY, _address);

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
    else if (configuredSource == Source::HTTP) {
        if (HttpPowerMeter.updateValues()) {
            std::lock_guard<std::mutex> l(_mutex);
            _powerMeter1Power = HttpPowerMeter.getPower(1);
            _powerMeter2Power = HttpPowerMeter.getPower(2);
            _powerMeter3Power = HttpPowerMeter.getPower(3);
            _lastPowerMeterUpdate = millis();
        }
    }
    else if (configuredSource == Source::SMAHM2) {
        std::lock_guard<std::mutex> l(_mutex);
        _powerMeter1Power = SMA_HM.getPowerL1();
        _powerMeter2Power = SMA_HM.getPowerL2();
        _powerMeter3Power = SMA_HM.getPowerL3();
        _lastPowerMeterUpdate = millis();
    }
}

bool PowerMeterClass::smlReadLoop()
{
    while (_upSmlSerial->available()) {
        double readVal = 0;
        unsigned char smlCurrentChar = _upSmlSerial->read();
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

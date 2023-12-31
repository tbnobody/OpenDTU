// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler, Malte Schmidt and others
 */
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "MqttHandlePowerLimiter.h"
#include "PowerLimiter.h"
#include <ctime>
#include <string>

MqttHandlePowerLimiterClass MqttHandlePowerLimiter;

void MqttHandlePowerLimiterClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandlePowerLimiterClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix() + "powerlimiter/cmd/mode";
    MqttSettings.subscribe(topic.c_str(), 0, std::bind(&MqttHandlePowerLimiterClass::onCmdMode, this, _1, _2, _3, _4, _5, _6));

    _lastPublish = millis();
}


void MqttHandlePowerLimiterClass::loop()
{
    std::unique_lock<std::mutex> mqttLock(_mqttMutex);

    const CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter.Enabled) {
        _mqttCallbacks.clear();
        return;
    }

    for (auto& callback : _mqttCallbacks) { callback(); }
    _mqttCallbacks.clear();

    mqttLock.unlock();

    if (!MqttSettings.getConnected() ) { return; }

    if ((millis() - _lastPublish) > (config.Mqtt.PublishInterval * 1000) ) {
        auto val = static_cast<unsigned>(PowerLimiter.getMode());
        MqttSettings.publish("powerlimiter/status/mode", String(val));

        yield();
        _lastPublish = millis();
    }
}


void MqttHandlePowerLimiterClass::onCmdMode(const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    std::string strValue(reinterpret_cast<const char*>(payload), len);
    int intValue = -1;
    try {
        intValue = std::stoi(strValue);
    }
    catch (std::invalid_argument const& e) {
        MessageOutput.printf("PowerLimiter MQTT handler: cannot parse payload of topic '%s' as int: %s\r\n",
                topic, strValue.c_str());
        return;
    }

    std::lock_guard<std::mutex> mqttLock(_mqttMutex);

    using Mode = PowerLimiterClass::Mode;
    switch (static_cast<Mode>(intValue)) {
        case Mode::UnconditionalFullSolarPassthrough:
            MessageOutput.println("Power limiter unconditional full solar PT");
            _mqttCallbacks.push_back(std::bind(&PowerLimiterClass::setMode,
                        &PowerLimiter, Mode::UnconditionalFullSolarPassthrough));
            break;
        case Mode::Disabled:
            MessageOutput.println("Power limiter disabled (override)");
            _mqttCallbacks.push_back(std::bind(&PowerLimiterClass::setMode,
                        &PowerLimiter, Mode::Disabled));
            break;
        case Mode::Normal:
            MessageOutput.println("Power limiter normal operation");
            _mqttCallbacks.push_back(std::bind(&PowerLimiterClass::setMode,
                        &PowerLimiter, Mode::Normal));
            break;
        default:
            MessageOutput.printf("PowerLimiter - unknown mode %d\r\n", intValue);
            break;
    }
}
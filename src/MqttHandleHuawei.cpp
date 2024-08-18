// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHandleHuawei.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "Huawei_can.h"
// #include "Failsafe.h"
#include "WebApi_Huawei.h"
#include <ctime>

MqttHandleHuaweiClass MqttHandleHuawei;

void MqttHandleHuaweiClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleHuaweiClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    String const& prefix = MqttSettings.getPrefix();

    auto subscribe = [&prefix, this](char const* subTopic, Topic t) {
        String fullTopic(prefix + "huawei/cmd/" + subTopic);
        MqttSettings.subscribe(fullTopic.c_str(), 0,
                std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, t,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5, std::placeholders::_6));
    };

    subscribe("limit_online_voltage", Topic::LimitOnlineVoltage);
    subscribe("limit_online_current", Topic::LimitOnlineCurrent);
    subscribe("limit_offline_voltage", Topic::LimitOfflineVoltage);
    subscribe("limit_offline_current", Topic::LimitOfflineCurrent);
    subscribe("mode", Topic::Mode);

    _lastPublish = millis();

}


void MqttHandleHuaweiClass::loop()
{
    const CONFIG_T& config = Configuration.get();

    std::unique_lock<std::mutex> mqttLock(_mqttMutex);

    if (!config.Huawei.Enabled) {
        _mqttCallbacks.clear();
        return;
    }

    for (auto& callback : _mqttCallbacks) { callback(); }
    _mqttCallbacks.clear();

    mqttLock.unlock();

    if (!MqttSettings.getConnected() ) {
        return;
    }

    const RectifierParameters_t *rp = HuaweiCan.get();

    if ((millis() - _lastPublish) > (config.Mqtt.PublishInterval * 1000) ) {
      MqttSettings.publish("huawei/data_age", String((millis() - HuaweiCan.getLastUpdate()) / 1000));
      MqttSettings.publish("huawei/input_voltage", String(rp->input_voltage));
      MqttSettings.publish("huawei/input_current", String(rp->input_current));
      MqttSettings.publish("huawei/input_power", String(rp->input_power));
      MqttSettings.publish("huawei/output_voltage", String(rp->output_voltage));
      MqttSettings.publish("huawei/output_current", String(rp->output_current));
      MqttSettings.publish("huawei/max_output_current", String(rp->max_output_current));
      MqttSettings.publish("huawei/output_power", String(rp->output_power));
      MqttSettings.publish("huawei/input_temp", String(rp->input_temp));
      MqttSettings.publish("huawei/output_temp", String(rp->output_temp));
      MqttSettings.publish("huawei/efficiency", String(rp->efficiency));
      MqttSettings.publish("huawei/mode", String(HuaweiCan.getMode()));


      yield();
      _lastPublish = millis();
    }
}


void MqttHandleHuaweiClass::onMqttMessage(Topic t,
        const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len,
        size_t index, size_t total)
{
    std::string strValue(reinterpret_cast<const char*>(payload), len);
    float payload_val = -1;
    try {
        payload_val = std::stof(strValue);
    }
    catch (std::invalid_argument const& e) {
        MessageOutput.printf("Huawei MQTT handler: cannot parse payload of topic '%s' as float: %s\r\n",
                topic, strValue.c_str());
        return;
    }

    std::lock_guard<std::mutex> mqttLock(_mqttMutex);

    switch (t) {
        case Topic::LimitOnlineVoltage:
            MessageOutput.printf("Limit Voltage: %f V\r\n", payload_val);
            _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                        &HuaweiCan, payload_val, HUAWEI_ONLINE_VOLTAGE));
            break;

        case Topic::LimitOfflineVoltage:
            MessageOutput.printf("Offline Limit Voltage: %f V\r\n", payload_val);
            _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                        &HuaweiCan, payload_val, HUAWEI_OFFLINE_VOLTAGE));
            break;

        case Topic::LimitOnlineCurrent:
            MessageOutput.printf("Limit Current: %f A\r\n", payload_val);
            _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                        &HuaweiCan, payload_val, HUAWEI_ONLINE_CURRENT));
            break;

        case Topic::LimitOfflineCurrent:
            MessageOutput.printf("Offline Limit Current: %f A\r\n", payload_val);
            _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                        &HuaweiCan, payload_val, HUAWEI_OFFLINE_CURRENT));
            break;

        case Topic::Mode:
            switch (static_cast<int>(payload_val)) {
                case 3:
                    MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Full internal control");
                    _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                                &HuaweiCan, HUAWEI_MODE_AUTO_INT));
                    break;

                case 2:
                    MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Internal on/off control, external power limit");
                    _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                                &HuaweiCan, HUAWEI_MODE_AUTO_EXT));
                    break;

                case 1:
                    MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Turned ON");
                    _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                                &HuaweiCan, HUAWEI_MODE_ON));
                    break;

                case 0:
                    MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Turned OFF");
                    _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                                &HuaweiCan, HUAWEI_MODE_OFF));
                    break;

                default:
                    MessageOutput.printf("[Huawei MQTT::] Invalid mode %.0f\r\n", payload_val);
                    break;
            }
            break;
    }
}

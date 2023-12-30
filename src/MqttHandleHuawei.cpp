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

#define TOPIC_SUB_LIMIT_ONLINE_VOLTAGE "limit_online_voltage"
#define TOPIC_SUB_LIMIT_ONLINE_CURRENT "limit_online_current"
#define TOPIC_SUB_LIMIT_OFFLINE_VOLTAGE "limit_offline_voltage"
#define TOPIC_SUB_LIMIT_OFFLINE_CURRENT "limit_offline_current"
#define TOPIC_SUB_MODE "mode"

MqttHandleHuaweiClass MqttHandleHuawei;

void MqttHandleHuaweiClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleHuaweiClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix();
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_ONLINE_VOLTAGE).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_ONLINE_CURRENT).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_OFFLINE_VOLTAGE).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_OFFLINE_CURRENT).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_MODE).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));

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


      yield();
      _lastPublish = millis();
    }
}


void MqttHandleHuaweiClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    const CONFIG_T& config = Configuration.get();

    char token_topic[MQTT_MAX_TOPIC_STRLEN + 40]; // respect all subtopics
    strncpy(token_topic, topic, MQTT_MAX_TOPIC_STRLEN + 40); // convert const char* to char*

    char* setting;
    char* rest = &token_topic[strlen(config.Mqtt.Topic)];

    strtok_r(rest, "/", &rest); // Remove "huawei"
    strtok_r(rest, "/", &rest); // Remove "cmd"
    
    setting = strtok_r(rest, "/", &rest);

    if (setting == NULL) {
        return;
    }

    char* strlimit = new char[len + 1];
    memcpy(strlimit, payload, len);
    strlimit[len] = '\0';
    float payload_val = strtof(strlimit, NULL);
    delete[] strlimit;

    std::lock_guard<std::mutex> mqttLock(_mqttMutex);

    if (!strcmp(setting, TOPIC_SUB_LIMIT_ONLINE_VOLTAGE)) {
        // Set voltage limit
        MessageOutput.printf("Limit Voltage: %f V\r\n", payload_val);
        _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                    &HuaweiCan, payload_val, HUAWEI_ONLINE_VOLTAGE));

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_OFFLINE_VOLTAGE)) {
        // Set current limit
        MessageOutput.printf("Offline Limit Voltage: %f V\r\n", payload_val);
        _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                    &HuaweiCan, payload_val, HUAWEI_OFFLINE_VOLTAGE));

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_ONLINE_CURRENT)) {
        // Set current limit
        MessageOutput.printf("Limit Current: %f A\r\n", payload_val);
        _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                    &HuaweiCan, payload_val, HUAWEI_ONLINE_CURRENT));

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_OFFLINE_CURRENT)) {
        // Set current limit
        MessageOutput.printf("Offline Limit Current: %f A\r\n", payload_val);
        _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setValue,
                    &HuaweiCan, payload_val, HUAWEI_OFFLINE_CURRENT));

    } else if (!strcmp(setting, TOPIC_SUB_MODE)) {
        // Control power on/off
        if(payload_val == 3) {
          MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Full internal control");
          _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                      &HuaweiCan, HUAWEI_MODE_AUTO_INT));
        }

        if(payload_val == 2) {
          MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Internal on/off control, external power limit");
          _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                      &HuaweiCan, HUAWEI_MODE_AUTO_EXT));
        }

        if(payload_val == 1) {
          MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Turned ON");
          _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                      &HuaweiCan, HUAWEI_MODE_ON));
        }
                 
        if(payload_val == 0) {
          MessageOutput.println("[Huawei MQTT::] Received MQTT msg. New mode: Turned OFF");
          _mqttCallbacks.push_back(std::bind(&HuaweiCanClass::setMode,
                      &HuaweiCan, HUAWEI_MODE_OFF));
        }
    } 
}
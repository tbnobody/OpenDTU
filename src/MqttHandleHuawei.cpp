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
#define TOPIC_SUB_POWER "power"

MqttHandleHuaweiClass MqttHandleHuawei;

void MqttHandleHuaweiClass::init()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix();
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_ONLINE_VOLTAGE).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_LIMIT_ONLINE_CURRENT).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "huawei/cmd/" + TOPIC_SUB_POWER).c_str(), 0, std::bind(&MqttHandleHuaweiClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));

    _lastPublish = millis();

}


void MqttHandleHuaweiClass::loop()
{
    if (!MqttSettings.getConnected() ) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (!config.Huawei_Enabled) {
        return;
    }

    const RectifierParameters_t *rp = HuaweiCan.get();

    if ((millis() - _lastPublish) > (config.Mqtt_PublishInterval * 1000) ) {
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

    // ignore messages if Huawei is disabled
    if (!config.Huawei_Enabled) {
        return;
    }

    char token_topic[MQTT_MAX_TOPIC_STRLEN + 40]; // respect all subtopics
    strncpy(token_topic, topic, MQTT_MAX_TOPIC_STRLEN + 40); // convert const char* to char*

    char* setting;
    char* rest = &token_topic[strlen(config.Mqtt_Topic)];

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

    if (!strcmp(setting, TOPIC_SUB_LIMIT_ONLINE_VOLTAGE)) {
        // Set voltage limit
        MessageOutput.printf("Limit Voltage: %f V\r\n", payload_val);
        HuaweiCan.setValue(payload_val, HUAWEI_ONLINE_VOLTAGE);

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_ONLINE_CURRENT)) {
        // Set current limit
        MessageOutput.printf("Limit Current: %f A\r\n", payload_val);
        HuaweiCan.setValue(payload_val, HUAWEI_ONLINE_CURRENT);
    } else if (!strcmp(setting, TOPIC_SUB_POWER)) {
        // Control power on/off
        MessageOutput.printf("Power: %f A\r\n", payload_val);
        if(payload_val > 0) {
          HuaweiCan.setPower(true);
        } else {
          HuaweiCan.setPower(false);
        }
    } 
}
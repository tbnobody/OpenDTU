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

    // FIXME - Some memory issue when this is enabled
    return;

    String json_string;
    const CONFIG_T& config = Configuration.get();
    const RectifierParameters_t *rp = HuaweiCan.get();

    if ((millis() - _lastPublish) > (config.Mqtt_PublishInterval * 1000) ) {
      DynamicJsonDocument doc(256);
      doc["data_age"] = (millis() - HuaweiCan.getLastUpdate()) / 1000;
      doc[F("input_voltage")] = rp->input_voltage;
      doc[F("input_current")] = rp->input_current;
      doc[F("input_power")] = rp->input_power;
      doc[F("output_voltage")] = rp->output_voltage;
      doc[F("output_current")] = rp->output_current;
      doc[F("max_output_current")] = rp->max_output_current;
      doc[F("output_power")] = rp->output_power;
      doc[F("input_temp")] = rp->input_temp;
      doc[F("output_temp")] = rp->output_temp;
      doc[F("efficiency")] = rp->efficiency;
      serializeJson(doc, json_string);
      MqttSettings.publish("huawei", json_string);

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
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler, Malte Schmidt and others
 */
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "MqttHandlePowerLimiter.h"
#include "PowerLimiter.h"
#include <ctime>

#define TOPIC_SUB_POWER_LIMITER "disable"

MqttHandlePowerLimiterClass MqttHandlePowerLimiter;

void MqttHandlePowerLimiterClass::init()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix();
    MqttSettings.subscribe(String(topic + "powerlimiter/cmd/" + TOPIC_SUB_POWER_LIMITER).c_str(), 0, std::bind(&MqttHandlePowerLimiterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));

    _lastPublish = millis();

}


void MqttHandlePowerLimiterClass::loop()
{
    if (!MqttSettings.getConnected() ) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if ((millis() - _lastPublish) > (config.Mqtt_PublishInterval * 1000) ) {
      MqttSettings.publish("powerlimiter/status/disabled", String(PowerLimiter.getDisable()));

      yield();
      _lastPublish = millis();
    }
}


void MqttHandlePowerLimiterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    const CONFIG_T& config = Configuration.get();

    char token_topic[MQTT_MAX_TOPIC_STRLEN + 40]; // respect all subtopics
    strncpy(token_topic, topic, MQTT_MAX_TOPIC_STRLEN + 40); // convert const char* to char*

    char* setting;
    char* rest = &token_topic[strlen(config.Mqtt_Topic)];

    strtok_r(rest, "/", &rest); // Remove "powerlimiter"
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

    if (!strcmp(setting, TOPIC_SUB_POWER_LIMITER)) {
        MessageOutput.printf("Disable power limter: %f A\r\n", payload_val);
        if(payload_val == 1) {
          PowerLimiter.setDisable(true);
        } 
        if(payload_val == 0) {
          PowerLimiter.setDisable(false);
        } 
    } 
}
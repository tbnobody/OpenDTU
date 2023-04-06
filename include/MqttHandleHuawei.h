// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Huawei_can.h>
#include <espMqttClient.h>

class MqttHandleHuaweiClass {
public:
    void init();
    void loop();

private:
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    uint32_t _lastPublishStats;
    uint32_t _lastPublish;

};

extern MqttHandleHuaweiClass MqttHandleHuawei;
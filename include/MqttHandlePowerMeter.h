// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "JsyMk.h"
#include <espMqttClient.h>

class MqttHandlePowerMeterClass {
public:
    using Field_t = JsyMkClass::Field_t;

    MqttHandlePowerMeterClass();
    void init(Scheduler& scheduler);

    static String getTopic(size_t channel, Field_t fieldId);

private:
    void
    loop();
    void publishField(size_t channel, const Field_t fieldId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total);

    Task _loopTask;
    uint32_t _lastUpdate = {};
};

extern MqttHandlePowerMeterClass MqttHandlePowerMeter;

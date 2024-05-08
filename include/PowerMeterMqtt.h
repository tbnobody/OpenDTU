// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterProvider.h"
#include <espMqttClient.h>
#include <map>
#include <mutex>

class PowerMeterMqtt : public PowerMeterProvider {
public:
    bool init() final;
    void deinit() final;
    void loop() final { }
    float getPowerTotal() const final;
    void doMqttPublish() const final;

private:
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    float _powerValueOne = 0;
    float _powerValueTwo = 0;
    float _powerValueThree = 0;

    std::map<String, float*> _mqttSubscriptions;

    mutable std::mutex _mutex;
};

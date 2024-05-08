// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterProvider.h"
#include <espMqttClient.h>
#include <vector>
#include <mutex>

class PowerMeterMqtt : public PowerMeterProvider {
public:
    bool init() final;
    void deinit() final;
    void loop() final { }
    float getPowerTotal() const final;
    void doMqttPublish() const final;

private:
    using MsgProperties = espMqttClientTypes::MessageProperties;
    void onMessage(MsgProperties const& properties, char const* topic,
            uint8_t const* payload, size_t len, size_t index,
            size_t total, float* targetVariable);

    float _powerValueOne = 0;
    float _powerValueTwo = 0;
    float _powerValueThree = 0;

    std::vector<String> _mqttSubscriptions;

    mutable std::mutex _mutex;
};

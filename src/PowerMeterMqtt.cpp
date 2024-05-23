// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterMqtt.h"
#include "MqttSettings.h"
#include "MessageOutput.h"

bool PowerMeterMqtt::init()
{
    auto subscribe = [this](char const* topic, float* targetVariable) {
        if (strlen(topic) == 0) { return; }
        MqttSettings.subscribe(topic, 0,
                std::bind(&PowerMeterMqtt::onMessage,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5, std::placeholders::_6,
                    targetVariable)
                );
        _mqttSubscriptions.push_back(topic);
    };

    subscribe(_cfg.Values[0].Topic, &_powerValueOne);
    subscribe(_cfg.Values[1].Topic, &_powerValueTwo);
    subscribe(_cfg.Values[2].Topic, &_powerValueThree);

    return _mqttSubscriptions.size() > 0;
}

PowerMeterMqtt::~PowerMeterMqtt()
{
    for (auto const& t: _mqttSubscriptions) { MqttSettings.unsubscribe(t); }
    _mqttSubscriptions.clear();
}

void PowerMeterMqtt::onMessage(PowerMeterMqtt::MsgProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len, size_t index,
        size_t total, float* targetVariable)
{
    std::string value(reinterpret_cast<char const*>(payload), len);
    try {
        std::lock_guard<std::mutex> l(_mutex);
        *targetVariable = std::stof(value);
    }
    catch (std::invalid_argument const& e) {
        MessageOutput.printf("[PowerMeterMqtt] cannot parse payload of topic "
                "'%s' as float: %s\r\n", topic, value.c_str());
        return;
    }

    if (_verboseLogging) {
        MessageOutput.printf("[PowerMeterMqtt] Updated from '%s', TotalPower: %5.2f\r\n",
                topic, getPowerTotal());
    }

    gotUpdate();
}

float PowerMeterMqtt::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    return _powerValueOne + _powerValueTwo + _powerValueThree;
}

void PowerMeterMqtt::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_mutex);
    mqttPublish("power1", _powerValueOne);
    mqttPublish("power2", _powerValueTwo);
    mqttPublish("power3", _powerValueThree);
}

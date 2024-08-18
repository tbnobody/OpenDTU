// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterProvider.h"
#include "MqttSettings.h"

bool PowerMeterProvider::isDataValid() const
{
    return _lastUpdate > 0 && ((millis() - _lastUpdate) < (30 * 1000));
}

void PowerMeterProvider::mqttPublish(String const& topic, float const& value) const
{
    MqttSettings.publish("powermeter/" + topic, String(value));
}

void PowerMeterProvider::mqttLoop() const
{
    if (!MqttSettings.getConnected()) { return; }

    if (!isDataValid()) { return; }

    auto constexpr halfOfAllMillis = std::numeric_limits<uint32_t>::max() / 2;
    if ((_lastUpdate - _lastMqttPublish) > halfOfAllMillis) { return; }

    mqttPublish("powertotal", getPowerTotal());

    doMqttPublish();

    _lastMqttPublish = millis();
}

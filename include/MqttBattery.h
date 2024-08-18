#pragma once

#include <optional>
#include "Battery.h"
#include <espMqttClient.h>

class MqttBattery : public BatteryProvider {
public:
    MqttBattery() = default;

    bool init(bool verboseLogging) final;
    void deinit() final;
    void loop() final { return; } // this class is event-driven
    std::shared_ptr<BatteryStats> getStats() const final { return _stats; }

private:
    bool _verboseLogging = false;
    String _socTopic;
    String _voltageTopic;
    std::shared_ptr<MqttBatteryStats> _stats = std::make_shared<MqttBatteryStats>();

    void onMqttMessageSoC(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len, size_t index, size_t total,
            char const* jsonPath);
    void onMqttMessageVoltage(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len, size_t index, size_t total,
            char const* jsonPath);
};

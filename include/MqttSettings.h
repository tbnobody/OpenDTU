#pragma once

#include <AsyncMqttClient.h>
#include <memory>

class MqttSettingsClass {
public:
    MqttSettingsClass();
    void init();

private:
    std::unique_ptr<AsyncMqttClient> mqttClient;
};

extern MqttSettingsClass MqttSettings;
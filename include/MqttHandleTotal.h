// #350 Publish Inverter Total Data via MQTT
#pragma once

#include <cstdint>

class MqttHandleTotalClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastPublish;
};

extern MqttHandleTotalClass MqttHandleTotal;
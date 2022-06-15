#pragma once

#include "Configuration.h"
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>

class MqttPublishingClass {
public:
    void init();
    void loop();

private:
    void publishField(String subtopic, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId);

    uint32_t _lastPublishStats[INV_MAX_COUNT];
};

extern MqttPublishingClass MqttPublishing;
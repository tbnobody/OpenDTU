// SPDX-License-Identifier: GPL-2.0-or-later
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
    void publishField(String subtopic, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic = "");

    uint32_t _lastPublishStats[INV_MAX_COUNT];
    uint32_t _lastPublish;
};

extern MqttPublishingClass MqttPublishing;
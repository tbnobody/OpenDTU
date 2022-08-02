// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "VeDirectFrameHandler.h"
#include "Configuration.h"
#include <Arduino.h>
#include <map>

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22
#endif

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21
#endif

class MqttVedirectPublishingClass {
public:
    void init();
    void loop();
private:
    std::map<String, String> _kv_map;
    VeDirectFrameHandler _myve;
    uint32_t _lastPublish;
};

extern MqttVedirectPublishingClass MqttVedirectPublishing;
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "VeDirectFrameHandler.h"
#include "Configuration.h"
#include <Arduino.h>

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX 22
#endif

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX 21
#endif

class MqttHandleVedirectClass {
public:
    void init();
    void loop();
private:
    veStruct _kvFrame;
    uint32_t _lastPublish;
};

extern MqttHandleVedirectClass MqttHandleVedirect;
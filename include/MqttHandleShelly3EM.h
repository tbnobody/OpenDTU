// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <HTTPClient.h>

class MqttHandleShelly3EMClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastPublish;
    HTTPClient http;
};

extern MqttHandleShelly3EMClass MqttHandleShelly3EM;
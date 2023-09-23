// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

class MqttHandleDtuClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastPublish = 0;
};

extern MqttHandleDtuClass MqttHandleDtu;
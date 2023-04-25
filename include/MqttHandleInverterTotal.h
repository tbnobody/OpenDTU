// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TimeoutHelper.h>

class MqttHandleInverterTotalClass {
public:
    void init();
    void loop();

private:
    TimeoutHelper _lastPublish;
};

extern MqttHandleInverterTotalClass MqttHandleInverterTotal;
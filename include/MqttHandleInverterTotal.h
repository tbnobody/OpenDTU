// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>

class MqttHandleInverterTotalClass {
public:
    MqttHandleInverterTotalClass();
    void init(Scheduler& scheduler);

private:
    void loop();

    Task _loopTask;
};

extern MqttHandleInverterTotalClass MqttHandleInverterTotal;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <cstdint>

class MqttHandleDtuClass {
public:
    MqttHandleDtuClass();
    void init(Scheduler& scheduler);

private:
    void loop();

    Task _loopTask;
};

extern MqttHandleDtuClass MqttHandleDtu;

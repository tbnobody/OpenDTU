// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include "MqttAvailableHandler.h"
#include <memory>

class MqttHandleInverterTotalClass {
public:
    MqttHandleInverterTotalClass();
    void init(Scheduler& scheduler);

private:
    void loop();
    bool isDataValid();
    void sendData();

    Task _loopTask;
    std::unique_ptr<MqttAvailableHandler> _availableHandler;
};

extern MqttHandleInverterTotalClass MqttHandleInverterTotal;

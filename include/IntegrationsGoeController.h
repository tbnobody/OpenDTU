// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <HTTPClient.h>
#include "NetworkSettings.h"
#include <TaskSchedulerDeclarations.h>

class IntegrationsGoeControllerClass {
public:
    IntegrationsGoeControllerClass();
    void init(Scheduler& scheduler);

private:
    void loop();
    void NetworkEvent(network_event event);

    Task _loopTask;

    bool _networkConnected = false;
    HTTPClient _http;
};

extern IntegrationsGoeControllerClass IntegrationsGoeController;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiWsConsoleClass {
public:
    WebApiWsConsoleClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    AsyncWebSocket _ws;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiWsConsoleClass {
public:
    WebApiWsConsoleClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);
    void reload();

private:
    AsyncWebSocket _ws;
    AsyncAuthenticationMiddleware _simpleDigestAuth;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();
};

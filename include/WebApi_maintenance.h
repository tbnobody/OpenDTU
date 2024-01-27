// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiMaintenanceClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onRebootPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

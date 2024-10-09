// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiSysstatusClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onSystemStatus(AsyncWebServerRequest* request);
};

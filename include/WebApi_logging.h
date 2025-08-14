// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiLoggingClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onLoggingAdminGet(AsyncWebServerRequest* request);
    void onLoggingAdminPost(AsyncWebServerRequest* request);
};

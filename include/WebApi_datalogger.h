// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiDataLoggerClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onDataLoggerAdminGet(AsyncWebServerRequest* request);
    void onDataLoggerAdminPost(AsyncWebServerRequest* request);
};

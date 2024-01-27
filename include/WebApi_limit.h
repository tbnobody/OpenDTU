// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiLimitClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onLimitStatus(AsyncWebServerRequest* request);
    void onLimitPost(AsyncWebServerRequest* request);
};

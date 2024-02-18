// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiDevInfoClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onDevInfoStatus(AsyncWebServerRequest* request);
};

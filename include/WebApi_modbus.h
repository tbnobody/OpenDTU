// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiModbusClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onModbusStatus(AsyncWebServerRequest* request);
    void onModbusAdminGet(AsyncWebServerRequest* request);
    void onModbusAdminPost(AsyncWebServerRequest* request);
};

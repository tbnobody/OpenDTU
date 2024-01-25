// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>


class WebApiPowerMeterClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onStatus(AsyncWebServerRequest* request);
    void onAdminGet(AsyncWebServerRequest* request);
    void onAdminPost(AsyncWebServerRequest* request);
    void onTestHttpRequest(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

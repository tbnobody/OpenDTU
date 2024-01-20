// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiDeviceClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onDeviceAdminGet(AsyncWebServerRequest* request);
    void onDeviceAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

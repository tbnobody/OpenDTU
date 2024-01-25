// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiSecurityClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onSecurityGet(AsyncWebServerRequest* request);
    void onSecurityPost(AsyncWebServerRequest* request);

    void onAuthenticateGet(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

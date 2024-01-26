// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiDtuClass {
public:
    WebApiDtuClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onDtuAdminGet(AsyncWebServerRequest* request);
    void onDtuAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;

    Task _applyDataTask;
    void applyDataTaskCb();
};

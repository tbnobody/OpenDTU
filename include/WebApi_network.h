// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiNetworkClass {
public:
    WebApiNetworkClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onNetworkStatus(AsyncWebServerRequest* request);
    void onNetworkAdminGet(AsyncWebServerRequest* request);
    void onNetworkAdminPost(AsyncWebServerRequest* request);

    Task _applyDataTask;
    void applyDataTaskCb();
};

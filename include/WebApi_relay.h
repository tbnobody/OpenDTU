// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiRelayClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onRelayGet(AsyncWebServerRequest* request);
    void onRelayPost(AsyncWebServerRequest* request);

    void onAuthenticateGet(AsyncWebServerRequest* request);
};

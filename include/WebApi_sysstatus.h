// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiSysstatusClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onSystemStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
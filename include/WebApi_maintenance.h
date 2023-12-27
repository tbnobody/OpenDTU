// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiMaintenanceClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onRebootPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
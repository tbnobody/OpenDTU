// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiPowerClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onPowerStatus(AsyncWebServerRequest* request);
    void onPowerPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
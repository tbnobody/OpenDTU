// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiMqttClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onMqttStatus(AsyncWebServerRequest* request);
    void onMqttAdminGet(AsyncWebServerRequest* request);
    void onMqttAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

class WebApiPylontechClass {
public:
    void init(AsyncWebServer* server);
    void loop();
    void getJsonData(JsonObject& root);

private:
    void onStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
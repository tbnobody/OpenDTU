// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiHuaweiClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onStatus(AsyncWebServerRequest* request);
    void onPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
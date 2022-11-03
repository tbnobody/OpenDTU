// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiSecurityClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onPasswordGet(AsyncWebServerRequest* request);
    void onPasswordPost(AsyncWebServerRequest* request);

    void onAuthenticateGet(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiNetworkClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onNetworkStatus(AsyncWebServerRequest* request);
    void onNetworkAdminGet(AsyncWebServerRequest* request);
    void onNetworkAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
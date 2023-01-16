// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiWireguardClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onWireguardStatus(AsyncWebServerRequest* request);
    void onWireguardAdminGet(AsyncWebServerRequest* request);
    void onWireguardAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
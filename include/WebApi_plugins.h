// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiPluginsClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onPluginList(AsyncWebServerRequest* request);
    void onPluginEdit(AsyncWebServerRequest* request);
    void onPluginRequest(AsyncWebServerRequest* request);
    AsyncWebServer* _server;
};
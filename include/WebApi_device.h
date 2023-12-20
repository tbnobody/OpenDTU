// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiDeviceClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onDeviceAdminGet(AsyncWebServerRequest* request);
    void onDeviceAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>


class WebApiPowerLimiterClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onStatus(AsyncWebServerRequest* request);
    void onAdminGet(AsyncWebServerRequest* request);
    void onAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
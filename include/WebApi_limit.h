// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiLimitClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onLimitStatus(AsyncWebServerRequest* request);
    void onLimitPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
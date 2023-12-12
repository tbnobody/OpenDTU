// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiEventlogClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onEventlogStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiDevInfoClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onDevInfoStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
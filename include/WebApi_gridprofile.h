// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiGridProfileClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onGridProfileStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
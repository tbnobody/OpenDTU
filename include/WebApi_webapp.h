// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiWebappClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    AsyncWebServer* _server;
};
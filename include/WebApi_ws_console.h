// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiWsConsoleClass {
public:
    WebApiWsConsoleClass();
    void init(AsyncWebServer& server);
    void loop();

private:
    AsyncWebServer* _server;
    AsyncWebSocket _ws;

    uint32_t _lastWsCleanup = 0;
};
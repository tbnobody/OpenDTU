// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>

class WebApiWsBatteryLiveClass {
public:
    WebApiWsBatteryLiveClass();
    void init(AsyncWebServer* server);
    void loop();

private:
    void generateJsonResponse(JsonVariant& root);
    void onLivedataStatus(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebServer* _server;
    AsyncWebSocket _ws;

    uint32_t _lastWsCleanup = 0;
    uint32_t _lastUpdateCheck = 0;
    static constexpr uint16_t _responseSize = 1024 + 512;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>

class WebApiWsHuaweiLiveClass {
public:
    WebApiWsHuaweiLiveClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);
    void reload();

private:
    void generateCommonJsonResponse(JsonVariant& root);
    void onLivedataStatus(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebServer* _server;
    AsyncWebSocket _ws;
    AuthenticationMiddleware _simpleDigestAuth;

    std::mutex _mutex;
    
    Task _wsCleanupTask;
    void wsCleanupTaskCb();

    Task _sendDataTask;
    void sendDataTaskCb();
};
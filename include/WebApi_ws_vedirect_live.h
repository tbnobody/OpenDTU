// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "ArduinoJson.h"
#include "Configuration.h"
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>
#include <VeDirectMpptController.h>
#include <mutex>

class WebApiWsVedirectLiveClass {
public:
    WebApiWsVedirectLiveClass();
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void generateJsonResponse(JsonVariant& root, bool fullUpdate);
    static void populateJson(const JsonObject &root, const VeDirectMpptController::data_t &mpptData);
    void onLivedataStatus(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
    bool hasUpdate(size_t idx);

    AsyncWebServer* _server;
    AsyncWebSocket _ws;

    uint32_t _lastFullPublish = 0;
    uint32_t _lastPublish = 0;
    uint16_t responseSize() const;

    std::mutex _mutex;

    Task _wsCleanupTask;
    void wsCleanupTaskCb();

    Task _sendDataTask;
    void sendDataTaskCb();
};

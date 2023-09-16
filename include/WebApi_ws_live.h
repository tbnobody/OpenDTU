// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>

class WebApiWsLiveClass {
public:
    WebApiWsLiveClass();
    void init(AsyncWebServer* server);
    void loop();

private:
    void generateJsonResponse(JsonVariant& root);
    void addField(JsonObject& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, String topic = "");
    void addTotalField(JsonObject& root, String name, float value, String unit, uint8_t digits);
    void onLivedataStatus(AsyncWebServerRequest* request);
    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebServer* _server;
    AsyncWebSocket _ws;

    uint32_t _lastWsPublish = 0;
    uint32_t _lastInvUpdateCheck = 0;
    uint32_t _lastWsCleanup = 0;
    uint32_t _newestInverterTimestamp = 0;

    std::mutex _mutex;
};
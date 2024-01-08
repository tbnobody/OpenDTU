// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_battery.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "Battery.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"
#include "Utils.h"

WebApiWsBatteryLiveClass::WebApiWsBatteryLiveClass()
    : _ws("/batterylivedata")
{
}

void WebApiWsBatteryLiveClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/batterylivedata/status", HTTP_GET, std::bind(&WebApiWsBatteryLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsBatteryLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsBatteryLiveClass::loop()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    if (millis() - _lastWsCleanup > 1000) {
        _ws.cleanupClients();
        _lastWsCleanup = millis();
    }

    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    if (!Battery.getStats()->updateAvailable(_lastUpdateCheck)) { return; }
    _lastUpdateCheck = millis();

    try {
        std::lock_guard<std::mutex> lock(_mutex);
        DynamicJsonDocument root(_responseSize);
         if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
            JsonVariant var = root;
            generateJsonResponse(var);

            String buffer;
            serializeJson(root, buffer);

            if (Configuration.get().Security.AllowReadonly) {
                _ws.setAuthentication("", "");
            } else {
                _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security.Password);
            }

            _ws.textAll(buffer);
        }
    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/batterylivedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
    } catch (const std::exception& exc) {
            MessageOutput.printf("Unknown exception in /api/batterylivedata/status. Reason: \"%s\".\r\n", exc.what());
    }
}

void WebApiWsBatteryLiveClass::generateJsonResponse(JsonVariant& root)
{
    Battery.getStats()->getLiveViewData(root);
}

void WebApiWsBatteryLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] connect\r\n", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] disconnect\r\n", server->url(), client->id());
    }
}

void WebApiWsBatteryLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse(false, _responseSize);
        auto& root = response->getRoot();
        generateJsonResponse(root);

        response->setLength();
        request->send(response);
    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/batterylivedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/batterylivedata/status. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}
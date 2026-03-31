// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_gridcharger.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include <gridcharger/Controller.h>
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"

#undef TAG
static const char* TAG = "webapi";

WebApiWsGridChargerLiveClass::WebApiWsGridChargerLiveClass()
    : _ws("/gridchargerlivedata")
{
}

void WebApiWsGridChargerLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/gridchargerlivedata/status", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiWsGridChargerLiveClass::onLivedataStatus, this, _1)));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsGridChargerLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.setCallback(std::bind(&WebApiWsGridChargerLiveClass::wsCleanupTaskCb, this));
    _wsCleanupTask.setIterations(TASK_FOREVER);
    _wsCleanupTask.setInterval(1 * TASK_SECOND);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.setCallback(std::bind(&WebApiWsGridChargerLiveClass::sendDataTaskCb, this));
    _sendDataTask.setIterations(TASK_FOREVER);
    _sendDataTask.setInterval(1 * TASK_SECOND);
    _sendDataTask.enable();

    _simpleDigestAuth.setUsername(AUTH_USERNAME);
    _simpleDigestAuth.setRealm("AC charger websocket");
    _simpleDigestAuth.setAuthType(AsyncAuthType::AUTH_DIGEST);

    reload();
}

void WebApiWsGridChargerLiveClass::reload()
{
    _ws.removeMiddleware(&_simpleDigestAuth);

    auto const& config = Configuration.get();

    if (config.Security.AllowReadonly) { return; }

    _ws.enable(false);
    _simpleDigestAuth.setPassword(config.Security.Password);
    _ws.addMiddleware(&_simpleDigestAuth);
    _ws.closeAll();
    _ws.enable(true);
}

void WebApiWsGridChargerLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

void WebApiWsGridChargerLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);
        JsonDocument root;
        JsonVariant var = root;

        generateCommonJsonResponse(var);

        if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
            String buffer;
            serializeJson(root, buffer);

            _ws.textAll(buffer);
        }
    } catch (std::bad_alloc& bad_alloc) {
        ESP_LOGE(TAG, "Calling /api/gridchargerlivedata/status has temporarily run out of resources. Reason: \"%s\".", bad_alloc.what());
    } catch (const std::exception& exc) {
        ESP_LOGE(TAG, "Unknown exception in /api/gridchargerlivedata/status. Reason: \"%s\".", exc.what());
    }
}

void WebApiWsGridChargerLiveClass::generateCommonJsonResponse(JsonVariant& root)
{
    GridCharger.getStats()->getLiveViewData(root);
}

void WebApiWsGridChargerLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        ESP_LOGD(TAG, "Websocket: [%s][%" PRIu32 "] connect", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        ESP_LOGD(TAG, "Websocket: [%s][%" PRIu32 "] disconnect", server->url(), client->id());
    }
}

void WebApiWsGridChargerLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        generateCommonJsonResponse(root);

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    } catch (std::bad_alloc& bad_alloc) {
        ESP_LOGE(TAG, "Calling /api/gridchargerlivedata/status has temporarily run out of resources. Reason: \"%s\".", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        ESP_LOGE(TAG, "Unknown exception in /api/gridchargerlivedata/status. Reason: \"%s\".", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

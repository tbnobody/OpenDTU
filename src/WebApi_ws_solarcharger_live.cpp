// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_solarcharger_live.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include "PowerLimiter.h"
#include <solarcharger/Controller.h>

#undef TAG
static const char* TAG = "webapi";

WebApiWsSolarChargerLiveClass::WebApiWsSolarChargerLiveClass()
    : _ws("/solarchargerlivedata")
{
}

void WebApiWsSolarChargerLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/solarchargerlivedata/status", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiWsSolarChargerLiveClass::onLivedataStatus, this, _1)));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsSolarChargerLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));


    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.setCallback(std::bind(&WebApiWsSolarChargerLiveClass::wsCleanupTaskCb, this));
    _wsCleanupTask.setIterations(TASK_FOREVER);
    _wsCleanupTask.setInterval(1 * TASK_SECOND);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.setCallback(std::bind(&WebApiWsSolarChargerLiveClass::sendDataTaskCb, this));
    _sendDataTask.setIterations(TASK_FOREVER);
    _sendDataTask.setInterval(500 * TASK_MILLISECOND);
    _sendDataTask.enable();

    _simpleDigestAuth.setUsername(AUTH_USERNAME);
    _simpleDigestAuth.setRealm("solarcharger websocket");
    _simpleDigestAuth.setAuthType(AsyncAuthType::AUTH_DIGEST);

    reload();
}

void WebApiWsSolarChargerLiveClass::reload()
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

void WebApiWsSolarChargerLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

void WebApiWsSolarChargerLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) { return; }

    // Update on ve.direct change or at least after 10 seconds
    bool fullUpdate = (millis() - _lastFullPublish > (10 * 1000));

    auto publishAgeMillis = millis() - _lastPublish;
    bool updateAvailable = SolarCharger.getStats()->getAgeMillis() < publishAgeMillis;

    if (fullUpdate || updateAvailable) {
        try {
            std::lock_guard<std::mutex> lock(_mutex);
            JsonDocument root;
            JsonVariant var = root;

            generateCommonJsonResponse(var, fullUpdate);

            if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
                String buffer;
                serializeJson(root, buffer);

                _ws.textAll(buffer);;
            }
        } catch (std::bad_alloc& bad_alloc) {
            ESP_LOGE(TAG, "Calling /api/solarchargerlivedata/status has temporarily run out of resources. Reason: \"%s\".", bad_alloc.what());
        } catch (const std::exception& exc) {
            ESP_LOGE(TAG, "Unknown exception in /api/solarchargerlivedata/status. Reason: \"%s\".", exc.what());
        }
    }

    if (fullUpdate) {
        _lastFullPublish = millis();
    }
}

void WebApiWsSolarChargerLiveClass::generateCommonJsonResponse(JsonVariant& root, bool fullUpdate)
{
    SolarCharger.getStats()->getLiveViewData(root, fullUpdate, _lastPublish);
    _lastPublish = millis();
}

void WebApiWsSolarChargerLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        ESP_LOGD(TAG, "Websocket: [%s][%" PRIu32 "] connect", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        ESP_LOGD(TAG, "Websocket: [%s][%" PRIu32 "] disconnect", server->url(), client->id());
    }
}

void WebApiWsSolarChargerLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse();
        auto& root = response->getRoot();

        generateCommonJsonResponse(root, true/*fullUpdate*/);

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
    } catch (std::bad_alloc& bad_alloc) {
        ESP_LOGE(TAG, "Calling /api/solarchargerlivedata/status has temporarily run out of resources. Reason: \"%s\".", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        ESP_LOGE(TAG, "Unknown exception in /api/solarchargerlivedata/status. Reason: \"%s\".", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

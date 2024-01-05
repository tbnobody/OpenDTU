// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_Huawei.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "Huawei_can.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"

WebApiWsHuaweiLiveClass::WebApiWsHuaweiLiveClass()
    : _ws("/huaweilivedata")
{
}

void WebApiWsHuaweiLiveClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/huaweilivedata/status", HTTP_GET, std::bind(&WebApiWsHuaweiLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsHuaweiLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsHuaweiLiveClass::loop()
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

    if (millis() - _lastUpdateCheck < 1000) {
        return;
    }
    _lastUpdateCheck = millis();

    try {
        String buffer;
        // free JsonDocument as soon as possible
        {
            DynamicJsonDocument root(1024);
            JsonVariant var = root;
            generateJsonResponse(var);
            serializeJson(root, buffer);
        }

        if (buffer) {
            if (Configuration.get().Security.AllowReadonly) {
                _ws.setAuthentication("", "");
            } else {
                _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security.Password);
            }

            _ws.textAll(buffer);
        }
    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/livedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
    }
}

void WebApiWsHuaweiLiveClass::generateJsonResponse(JsonVariant& root)
{
    const RectifierParameters_t * rp = HuaweiCan.get();

    root["data_age"] = (millis() - HuaweiCan.getLastUpdate()) / 1000;
    root["input_voltage"]["v"] = rp->input_voltage;
    root["input_voltage"]["u"] = "V";
    root["input_current"]["v"] = rp->input_current;
    root["input_current"]["u"] = "A";
    root["input_power"]["v"] = rp->input_power;
    root["input_power"]["u"] = "W";
    root["output_voltage"]["v"] = rp->output_voltage;
    root["output_voltage"]["u"] = "V";
    root["output_current"]["v"] = rp->output_current;
    root["output_current"]["u"] = "A";
    root["max_output_current"]["v"] = rp->max_output_current;
    root["max_output_current"]["u"] = "A";
    root["output_power"]["v"] = rp->output_power;
    root["output_power"]["u"] = "W";
    root["input_temp"]["v"] = rp->input_temp;
    root["input_temp"]["u"] = "°C";
    root["output_temp"]["v"] = rp->output_temp;
    root["output_temp"]["u"] = "°C";
    root["efficiency"]["v"] = rp->efficiency * 100;
    root["efficiency"]["u"] = "%";

}

void WebApiWsHuaweiLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] connect", server->url(), client->id());
        Serial.println(str);
        MessageOutput.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] disconnect", server->url(), client->id());
        Serial.println(str);
        MessageOutput.println(str);
    }
}

void WebApiWsHuaweiLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, 1024U);
        JsonVariant root = response->getRoot().as<JsonVariant>();
        generateJsonResponse(root);

        response->setLength();
        request->send(response);
    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/livedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());

        WebApi.sendTooManyRequests(request);
    }
}
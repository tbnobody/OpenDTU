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

void WebApiWsHuaweiLiveClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = server;
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

    DynamicJsonDocument root(1024);
    JsonVariant var = root;
    generateJsonResponse(var);

    String buffer;
    if (buffer) {
        serializeJson(root, buffer);

        if (Configuration.get().Security_AllowReadonly) {
            _ws.setAuthentication("", "");
        } else {
            _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security_Password);
        }

        _ws.textAll(buffer);
    }

    
}

void WebApiWsHuaweiLiveClass::generateJsonResponse(JsonVariant& root)
{
    const RectifierParameters_t& rp = HuaweiCan.get();

    root["data_age"] = (millis() - HuaweiCan.getLastUpdate()) / 1000;
    root[F("input_voltage")]["v"] = rp.input_voltage;
    root[F("input_voltage")]["u"] = "V";
    root[F("input_current")]["v"] = rp.input_current;
    root[F("input_current")]["u"] = "A";
    root[F("input_power")]["v"] = rp.input_power;
    root[F("input_power")]["u"] = "W";
    root[F("output_voltage")]["v"] = rp.output_voltage;
    root[F("output_voltage")]["u"] = "V";
    root[F("output_current")]["v"] = rp.output_current;
    root[F("output_current")]["u"] = "A";
    root[F("max_output_current")]["v"] = rp.max_output_current;
    root[F("max_output_current")]["u"] = "A";
    root[F("output_power")]["v"] = rp.output_power;
    root[F("output_power")]["u"] = "W";
    root[F("input_temp")]["v"] = rp.input_temp;
    root[F("input_temp")]["u"] = "°C";
    root[F("output_temp")]["v"] = rp.output_temp;
    root[F("output_temp")]["u"] = "°C";
    root[F("efficiency")]["v"] = rp.efficiency;
    root[F("efficiency")]["u"] = "%";

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
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 1024U);
    JsonVariant root = response->getRoot().as<JsonVariant>();
    generateJsonResponse(root);

    response->setLength();
    request->send(response);
}
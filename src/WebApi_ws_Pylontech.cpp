// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_Pylontech.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "Battery.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"

WebApiWsPylontechLiveClass::WebApiWsPylontechLiveClass()
    : _ws("/batterylivedata")
{
}

void WebApiWsPylontechLiveClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = server;
    _server->on("/api/batterylivedata/status", HTTP_GET, std::bind(&WebApiWsPylontechLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsPylontechLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsPylontechLiveClass::loop()
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

void WebApiWsPylontechLiveClass::generateJsonResponse(JsonVariant& root)
{
    root["data_age"] = (millis() - Battery.lastUpdate) / 1000;

    root[F("chargeVoltage")]["v"] = Battery.chargeVoltage ;
    root[F("chargeVoltage")]["u"] = "V";
    root[F("chargeCurrentLimitation")]["v"] = Battery.chargeCurrentLimitation ;
    root[F("chargeCurrentLimitation")]["u"] = "A";
    root[F("dischargeCurrentLimitation")]["v"] = Battery.dischargeCurrentLimitation ;
    root[F("dischargeCurrentLimitation")]["u"] = "A";
    root[F("stateOfCharge")]["v"] = Battery.stateOfCharge ;
    root[F("stateOfCharge")]["u"] = "%";
    root[F("stateOfHealth")]["v"] = Battery.stateOfHealth ;
    root[F("stateOfHealth")]["u"] = "%";
    root[F("voltage")]["v"] = Battery.voltage;
    root[F("voltage")]["u"] = "V";
    root[F("current")]["v"] = Battery.current ;
    root[F("current")]["u"] = "A";
    root[F("temperature")]["v"] = Battery.temperature ;
    root[F("temperature")]["u"] = "°C";

    // Alarms
    root["alarms"][F("dischargeCurrent")] = Battery.alarmOverCurrentDischarge ;
    root["alarms"][F("chargeCurrent")] = Battery.alarmOverCurrentCharge ;
    root["alarms"][F("lowTemperature")] = Battery.alarmUnderTemperature ;
    root["alarms"][F("highTemperature")] = Battery.alarmOverTemperature ;
    root["alarms"][F("lowVoltage")] = Battery.alarmUnderVoltage ;
    root["alarms"][F("highVoltage")] = Battery.alarmOverVoltage ;
    root["alarms"][F("bmsInternal")] = Battery.alarmBmsInternal ;

    // Warnings
    root["warnings"][F("dischargeCurrent")] = Battery.warningHighCurrentDischarge ;
    root["warnings"][F("chargeCurrent")] = Battery.warningHighCurrentCharge ;
    root["warnings"][F("lowTemperature")] = Battery.warningLowTemperature ;
    root["warnings"][F("highTemperature")] = Battery.warningHighTemperature ;
    root["warnings"][F("lowVoltage")] = Battery.warningLowVoltage ;
    root["warnings"][F("highVoltage")] = Battery.warningHighVoltage ;
    root["warnings"][F("bmsInternal")] = Battery.warningBmsInternal ;

    // Misc
    root[F("manufacturer")] = Battery.manufacturer ;
    root[F("chargeEnabled")] = Battery.chargeEnabled ;
    root[F("dischargeEnabled")] = Battery.dischargeEnabled ;
    root[F("chargeImmediately")] = Battery.chargeImmediately ;


}

void WebApiWsPylontechLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
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

void WebApiWsPylontechLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
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
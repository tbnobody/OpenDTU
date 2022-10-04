// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "AsyncJson.h"
#include "Configuration.h"

WebApiWsLiveClass::WebApiWsLiveClass()
    : _ws("/livedata")
{
}

void WebApiWsLiveClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = server;
    _server->on("/api/livedata/status", HTTP_GET, std::bind(&WebApiWsLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsLiveClass::loop()
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

    if (millis() - _lastInvUpdateCheck < 1000) {
        return;
    }
    _lastInvUpdateCheck = millis();

    uint32_t maxTimeStamp = 0;
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        if (inv->Statistics()->getLastUpdate() > maxTimeStamp) {
            maxTimeStamp = inv->Statistics()->getLastUpdate();
        }
    }

    // Update on every inverter change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || (maxTimeStamp != _newestInverterTimestamp)) {

        DynamicJsonDocument root(40960);
        JsonVariant var = root;
        generateJsonResponse(var);

        String buffer;
        if (buffer) {
            serializeJson(root, buffer);
            _ws.textAll(buffer);
        }

        _lastWsPublish = millis();
    }
}

void WebApiWsLiveClass::generateJsonResponse(JsonVariant& root)
{
    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        char buffer[sizeof(uint64_t) * 8 + 1];
        snprintf(buffer, sizeof(buffer), "%0x%08x",
            ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

        root[i][F("serial")] = String(buffer);
        root[i][F("name")] = inv->name();
        root[i][F("data_age")] = (millis() - inv->Statistics()->getLastUpdate()) / 1000;
        root[i][F("reachable")] = inv->isReachable();
        root[i][F("producing")] = inv->isProducing();

        // Loop all channels
        for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
            addField(root, i, inv, c, FLD_PAC);
            addField(root, i, inv, c, FLD_UAC);
            addField(root, i, inv, c, FLD_IAC);
            if (c == 0) {
                addField(root, i, inv, c, FLD_PDC, F("Power DC"));
            } else {
                addField(root, i, inv, c, FLD_PDC);
            }
            addField(root, i, inv, c, FLD_UDC);
            addField(root, i, inv, c, FLD_IDC);
            addField(root, i, inv, c, FLD_YD);
            addField(root, i, inv, c, FLD_YT);
            addField(root, i, inv, c, FLD_F);
            addField(root, i, inv, c, FLD_T);
            addField(root, i, inv, c, FLD_PCT);
            addField(root, i, inv, c, FLD_PRA);
            addField(root, i, inv, c, FLD_EFF);
            addField(root, i, inv, c, FLD_IRR);
        }

        if (inv->Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
            root[i][F("events")] = inv->EventLog()->getEntryCount();
        } else {
            root[i][F("events")] = -1;
        }

        if (inv->Statistics()->getLastUpdate() > _newestInverterTimestamp) {
            _newestInverterTimestamp = inv->Statistics()->getLastUpdate();
        }
    }
}

void WebApiWsLiveClass::addField(JsonVariant& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic)
{
    if (inv->Statistics()->hasChannelFieldValue(channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->Statistics()->getChannelFieldName(channel, fieldId);
        } else {
            chanName = topic;
        }
        root[idx][String(channel)][chanName]["v"] = inv->Statistics()->getChannelFieldValue(channel, fieldId);
        root[idx][String(channel)][chanName]["u"] = inv->Statistics()->getChannelFieldUnit(channel, fieldId);
    }
}

void WebApiWsLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] connect", server->url(), client->id());
        Serial.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] disconnect", server->url(), client->id());
        Serial.println(str);
    }
}

void WebApiWsLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse(true, 40960U);
    JsonVariant root = response->getRoot().as<JsonVariant>();

    generateJsonResponse(root);

    response->setLength();
    request->send(response);
}
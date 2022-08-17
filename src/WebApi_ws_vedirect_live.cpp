// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_vedirect_live.h"
#include "AsyncJson.h"
#include "Configuration.h"

WebApiWsVedirectLiveClass::WebApiWsVedirectLiveClass()
    : _ws("/vedirectlivedata")
{
}

void WebApiWsVedirectLiveClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;
    _server->on("/api/vedirectlivedata/status", HTTP_GET, std::bind(&WebApiWsVedirectLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsVedirectLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));
}

void WebApiWsVedirectLiveClass::loop()
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

    if (millis() - _lastVedirectUpdateCheck < 1000) {
        return;
    }
    _lastVedirectUpdateCheck = millis();

    uint32_t maxTimeStamp = 0;
    if (VeDirect.getLastUpdate() > maxTimeStamp) {
        maxTimeStamp = VeDirect.getLastUpdate();
    }

    // Update on ve.direct change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || (maxTimeStamp != _newestVedirectTimestamp)) {

        DynamicJsonDocument root(40960);
        JsonVariant var = root;
        generateJsonResponse(var);

        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer* buffer = _ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char*)buffer->get(), len + 1);
            _ws.textAll(buffer);
        }

        _lastWsPublish = millis();
    }
}

void WebApiWsVedirectLiveClass::generateJsonResponse(JsonVariant& root)
{
    root[F("data_age")] = (millis() - VeDirect.getLastUpdate() ) / 1000;
    root[F("age_critical")] = ((millis() - VeDirect.getLastUpdate()) / 1000) > Configuration.get().Vedirect_PollInterval * 5;

    for ( int i = 0; i < VeDirect.veEnd; i++ ) {
        if(strcmp(VeDirect.veName[i], "PID") == 0) {
            root[F(VeDirect.veName[i])] = VeDirect.getPidAsString(VeDirect.veValue[i]);
        }
        else if(strcmp(VeDirect.veName[i], "SER#") == 0) {
            root[F("SER")] = VeDirect.veValue[i];
        } 
        else if(strcmp(VeDirect.veName[i], "CS") == 0) {
            root[F(VeDirect.veName[i])] = VeDirect.getCsAsString(VeDirect.veValue[i]);
        } 
        else if(strcmp(VeDirect.veName[i], "ERR") == 0) {
            root[F(VeDirect.veName[i])] = VeDirect.getErrAsString(VeDirect.veValue[i]);
        } 
        else if(strcmp(VeDirect.veName[i], "OR") == 0) {
            root[F(VeDirect.veName[i])] = VeDirect.getOrAsString(VeDirect.veValue[i]);
        } 
         else if(strcmp(VeDirect.veName[i], "MPPT") == 0) {
            root[F(VeDirect.veName[i])] = VeDirect.getMpptAsString(VeDirect.veValue[i]);
        } 
        else if((strcmp(VeDirect.veName[i], "V") == 0) || (strcmp(VeDirect.veName[i], "VPV") == 0)) {
            root[F(VeDirect.veName[i])]["v"] = round(std::stod(VeDirect.veValue[i]) / 10.0) / 100.0;
            root[F(VeDirect.veName[i])]["u"] = "V";
        } 
        else if(strcmp(VeDirect.veName[i], "I") == 0) {
            root[F(VeDirect.veName[i])]["v"] = round(std::stod(VeDirect.veValue[i]) / 10.0) / 100.0;
            root[F(VeDirect.veName[i])]["u"] = "A";
        } 
        else if((strcmp(VeDirect.veName[i], "PPV") == 0) || (strcmp(VeDirect.veName[i], "H21") == 0) || (strcmp(VeDirect.veName[i], "H23") == 0)){
            root[F(VeDirect.veName[i])]["v"] = std::stoi(VeDirect.veValue[i]);
            root[F(VeDirect.veName[i])]["u"] = "W";
        } 
        else if((strcmp(VeDirect.veName[i], "H19") == 0) || (strcmp(VeDirect.veName[i], "H20") == 0) || (strcmp(VeDirect.veName[i], "H22") == 0)){
            root[F(VeDirect.veName[i])]["v"] = std::stod(VeDirect.veValue[i]) / 100.0;
            root[F(VeDirect.veName[i])]["u"] = "kWh";
        } 
        else if(strcmp(VeDirect.veName[i], "HSDS") == 0){
            root[F(VeDirect.veName[i])]["v"] = std::stoi(VeDirect.veValue[i]);
            root[F(VeDirect.veName[i])]["u"] = "Days";
        } 
        else {
            root[F(VeDirect.veName[i])] = VeDirect.veValue[i];
        }
    }

    if (VeDirect.getLastUpdate() > _newestVedirectTimestamp) {
        _newestVedirectTimestamp = VeDirect.getLastUpdate();
    }
}

void WebApiWsVedirectLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        char str[64];
        sprintf(str, "Websocket: [%s][%u] connect", server->url(), client->id());
        Serial.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        sprintf(str, "Websocket: [%s][%u] disconnect", server->url(), client->id());
        Serial.println(str);
    }
}

void WebApiWsVedirectLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 40960U);
    JsonVariant root = response->getRoot().as<JsonVariant>();
    generateJsonResponse(root);

    response->setLength();
    request->send(response);
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "defaults.h"

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
    JsonArray invArray = root.createNestedArray("inverters");

    float totalPower = 0;
    float totalYieldDay = 0;
    float totalYieldTotal = 0;

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        JsonObject invObject = invArray.createNestedObject();

        invObject[F("serial")] = inv->serialString();
        invObject[F("name")] = inv->name();
        invObject[F("data_age")] = (millis() - inv->Statistics()->getLastUpdate()) / 1000;
        invObject[F("reachable")] = inv->isReachable();
        invObject[F("producing")] = inv->isProducing();
        invObject[F("limit_relative")] = inv->SystemConfigPara()->getLimitPercent();
        if (inv->DevInfo()->getMaxPower() > 0) {
            invObject[F("limit_absolute")] = inv->SystemConfigPara()->getLimitPercent() * inv->DevInfo()->getMaxPower() / 100.0;
        } else {
            invObject[F("limit_absolute")] = -1;
        }

        // Loop all channels
        for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
            if (c > 0) {
                INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
                if (inv_cfg != nullptr) {
                    invObject[String(c)][F("name")]["u"] = inv_cfg->channel[c - 1].Name;
                }
            }
            addField(invObject, i, inv, c, FLD_PAC);
            addField(invObject, i, inv, c, FLD_UAC);
            addField(invObject, i, inv, c, FLD_IAC);
            if (c == 0) {
                addField(invObject, i, inv, c, FLD_PDC, F("Power DC"));
            } else {
                addField(invObject, i, inv, c, FLD_PDC);
            }
            addField(invObject, i, inv, c, FLD_UDC);
            addField(invObject, i, inv, c, FLD_IDC);
            addField(invObject, i, inv, c, FLD_YD);
            addField(invObject, i, inv, c, FLD_YT);
            addField(invObject, i, inv, c, FLD_F);
            addField(invObject, i, inv, c, FLD_T);
            addField(invObject, i, inv, c, FLD_PF);
            addField(invObject, i, inv, c, FLD_PRA);
            addField(invObject, i, inv, c, FLD_EFF);
            if (c > 0 && inv->Statistics()->getChannelMaxPower(c - 1) > 0) {
                addField(invObject, i, inv, c, FLD_IRR);
            }
        }

        if (inv->Statistics()->hasChannelFieldValue(CH0, FLD_EVT_LOG)) {
            invObject[F("events")] = inv->EventLog()->getEntryCount();
        } else {
            invObject[F("events")] = -1;
        }

        if (inv->Statistics()->getLastUpdate() > _newestInverterTimestamp) {
            _newestInverterTimestamp = inv->Statistics()->getLastUpdate();
        }

        totalPower += inv->Statistics()->getChannelFieldValue(CH0, FLD_PAC);
        totalYieldDay += inv->Statistics()->getChannelFieldValue(CH0, FLD_YD);
        totalYieldTotal += inv->Statistics()->getChannelFieldValue(CH0, FLD_YT);
    }

    JsonObject totalObj = root.createNestedObject("total");
    // todo: Fixed hard coded name, unit and digits
    addTotalField(totalObj, "Power", totalPower, "W", 1);
    addTotalField(totalObj, "YieldDay", totalYieldDay, "Wh", 0);
    addTotalField(totalObj, "YieldTotal", totalYieldTotal, "kWh", 2);

    JsonObject hintObj = root.createNestedObject("hints");
    struct tm timeinfo;
    hintObj[F("time_sync")] = !getLocalTime(&timeinfo, 5);
    hintObj[F("radio_problem")] = (!Hoymiles.getRadio()->isConnected() || !Hoymiles.getRadio()->isPVariant());
    if (!strcmp(Configuration.get().Security_Password, ACCESS_POINT_PASSWORD)) {
        hintObj[F("default_password")] = true;
    } else {
        hintObj[F("default_password")] = false;
    }
}

void WebApiWsLiveClass::addField(JsonObject& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic)
{
    if (inv->Statistics()->hasChannelFieldValue(channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->Statistics()->getChannelFieldName(channel, fieldId);
        } else {
            chanName = topic;
        }
        root[String(channel)][chanName]["v"] = inv->Statistics()->getChannelFieldValue(channel, fieldId);
        root[String(channel)][chanName]["u"] = inv->Statistics()->getChannelFieldUnit(channel, fieldId);
        root[String(channel)][chanName]["d"] = inv->Statistics()->getChannelFieldDigits(channel, fieldId);
    }
}

void WebApiWsLiveClass::addTotalField(JsonObject& root, String name, float value, String unit, uint8_t digits)
{
    root[name]["v"] = value;
    root[name]["u"] = unit;
    root[name]["d"] = digits;
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
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 40960U);
    JsonVariant root = response->getRoot();

    generateJsonResponse(root);

    response->setLength();
    request->send(response);
}
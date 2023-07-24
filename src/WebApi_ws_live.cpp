// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>

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

        try {
            DynamicJsonDocument root(40960);
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

        } catch (const std::bad_alloc& bad_alloc) {
            MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        } catch (const std::exception& exc) {
            MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
        }

        _lastWsPublish = millis();
    }
}

void WebApiWsLiveClass::generateJsonResponse(JsonVariant& root)
{
    JsonArray invArray = root.createNestedArray("inverters");

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        JsonObject invObject = invArray.createNestedObject();
        INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
        if (inv_cfg == nullptr) {
            continue;
        }

        invObject["serial"] = inv->serialString();
        invObject["name"] = inv->name();
        invObject["order"] = inv_cfg->Order;
        invObject["data_age"] = (millis() - inv->Statistics()->getLastUpdate()) / 1000;
        invObject["poll_enabled"] = inv->getEnablePolling();
        invObject["reachable"] = inv->isReachable();
        invObject["producing"] = inv->isProducing();
        invObject["limit_relative"] = inv->SystemConfigPara()->getLimitPercent();
        if (inv->DevInfo()->getMaxPower() > 0) {
            invObject["limit_absolute"] = inv->SystemConfigPara()->getLimitPercent() * inv->DevInfo()->getMaxPower() / 100.0;
        } else {
            invObject["limit_absolute"] = -1;
        }

        // Loop all channels
        for (auto& t : inv->Statistics()->getChannelTypes()) {
            JsonObject chanTypeObj = invObject.createNestedObject(inv->Statistics()->getChannelTypeName(t));
            for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                if (t == TYPE_DC) {
                    chanTypeObj[String(static_cast<uint8_t>(c))]["name"]["u"] = inv_cfg->channel[c].Name;
                }
                addField(chanTypeObj, i, inv, t, c, FLD_PAC);
                addField(chanTypeObj, i, inv, t, c, FLD_UAC);
                addField(chanTypeObj, i, inv, t, c, FLD_IAC);
                if (t == TYPE_AC) {
                    addField(chanTypeObj, i, inv, t, c, FLD_PDC, "Power DC");
                } else {
                    addField(chanTypeObj, i, inv, t, c, FLD_PDC);
                }
                addField(chanTypeObj, i, inv, t, c, FLD_UDC);
                addField(chanTypeObj, i, inv, t, c, FLD_IDC);
                addField(chanTypeObj, i, inv, t, c, FLD_YD);
                addField(chanTypeObj, i, inv, t, c, FLD_YT);
                addField(chanTypeObj, i, inv, t, c, FLD_F);
                addField(chanTypeObj, i, inv, t, c, FLD_T);
                addField(chanTypeObj, i, inv, t, c, FLD_PF);
                addField(chanTypeObj, i, inv, t, c, FLD_Q);
                addField(chanTypeObj, i, inv, t, c, FLD_EFF);
                if (t == TYPE_DC && inv->Statistics()->getStringMaxPower(c) > 0) {
                    addField(chanTypeObj, i, inv, t, c, FLD_IRR);
                }
            }
        }

        if (inv->Statistics()->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
            invObject["events"] = inv->EventLog()->getEntryCount();
        } else {
            invObject["events"] = -1;
        }

        if (inv->Statistics()->getLastUpdate() > _newestInverterTimestamp) {
            _newestInverterTimestamp = inv->Statistics()->getLastUpdate();
        }
    }

    JsonObject totalObj = root.createNestedObject("total");
    addTotalField(totalObj, "Power", Datastore.getTotalAcPowerEnabled(), "W", Datastore.getTotalAcPowerDigits());
    addTotalField(totalObj, "YieldDay", Datastore.getTotalAcYieldDayEnabled(), "Wh", Datastore.getTotalAcYieldDayDigits());
    addTotalField(totalObj, "YieldTotal", Datastore.getTotalAcYieldTotalEnabled(), "kWh", Datastore.getTotalAcYieldTotalDigits());

    JsonObject hintObj = root.createNestedObject("hints");
    struct tm timeinfo;
    hintObj["time_sync"] = !getLocalTime(&timeinfo, 5);
    hintObj["radio_problem"] = (Hoymiles.getRadioNrf()->isInitialized() && (!Hoymiles.getRadioNrf()->isConnected() || !Hoymiles.getRadioNrf()->isPVariant())) || (Hoymiles.getRadioCmt()->isInitialized() && (!Hoymiles.getRadioCmt()->isConnected()));
    if (!strcmp(Configuration.get().Security_Password, ACCESS_POINT_PASSWORD)) {
        hintObj["default_password"] = true;
    } else {
        hintObj["default_password"] = false;
    }
}

void WebApiWsLiveClass::addField(JsonObject& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId, String topic)
{
    if (inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
        String chanName;
        if (topic == "") {
            chanName = inv->Statistics()->getChannelFieldName(type, channel, fieldId);
        } else {
            chanName = topic;
        }
        String chanNum;
        chanNum = channel;
        root[chanNum][chanName]["v"] = inv->Statistics()->getChannelFieldValue(type, channel, fieldId);
        root[chanNum][chanName]["u"] = inv->Statistics()->getChannelFieldUnit(type, channel, fieldId);
        root[chanNum][chanName]["d"] = inv->Statistics()->getChannelFieldDigits(type, channel, fieldId);
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
        MessageOutput.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        snprintf(str, sizeof(str), "Websocket: [%s][%u] disconnect", server->url(), client->id());
        MessageOutput.println(str);
    }
}

void WebApiWsLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        AsyncJsonResponse* response = new AsyncJsonResponse(false, 40960U);
        JsonVariant root = response->getRoot();

        generateJsonResponse(root);

        response->setLength();
        request->send(response);

    } catch (const std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/livedata/status temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/livedata/status. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}
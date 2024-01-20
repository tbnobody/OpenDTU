// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_live.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>

WebApiWsLiveClass::WebApiWsLiveClass()
    : _ws("/livedata")
    , _wsCleanupTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::wsCleanupTaskCb, this))
    , _sendDataTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsLiveClass::sendDataTaskCb, this))
{
}

void WebApiWsLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/livedata/status", HTTP_GET, std::bind(&WebApiWsLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.enable();
}

void WebApiWsLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();

    if (Configuration.get().Security.AllowReadonly) {
        _ws.setAuthentication("", "");
    } else {
        _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security.Password);
    }
}

void WebApiWsLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    uint32_t maxTimeStamp = 0;
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        maxTimeStamp = std::max<uint32_t>(maxTimeStamp, inv->Statistics()->getLastUpdate());
    }

    // Update on every inverter change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || (maxTimeStamp != _newestInverterTimestamp)) {

        try {
            std::lock_guard<std::mutex> lock(_mutex);
            DynamicJsonDocument root(4096 * INV_MAX_COUNT);
            if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
                JsonVariant var = root;
                generateJsonResponse(var);

                String buffer;
                serializeJson(root, buffer);

                _ws.textAll(buffer);
                _newestInverterTimestamp = maxTimeStamp;
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
                addField(chanTypeObj, inv, t, c, FLD_PAC);
                addField(chanTypeObj, inv, t, c, FLD_UAC);
                addField(chanTypeObj, inv, t, c, FLD_IAC);
                if (t == TYPE_AC) {
                    addField(chanTypeObj, inv, t, c, FLD_PDC, "Power DC");
                } else {
                    addField(chanTypeObj, inv, t, c, FLD_PDC);
                }
                addField(chanTypeObj, inv, t, c, FLD_UDC);
                addField(chanTypeObj, inv, t, c, FLD_IDC);
                addField(chanTypeObj, inv, t, c, FLD_YD);
                addField(chanTypeObj, inv, t, c, FLD_YT);
                addField(chanTypeObj, inv, t, c, FLD_F);
                addField(chanTypeObj, inv, t, c, FLD_T);
                addField(chanTypeObj, inv, t, c, FLD_PF);
                addField(chanTypeObj, inv, t, c, FLD_Q);
                addField(chanTypeObj, inv, t, c, FLD_EFF);
                if (t == TYPE_DC && inv->Statistics()->getStringMaxPower(c) > 0) {
                    addField(chanTypeObj, inv, t, c, FLD_IRR);
                    chanTypeObj[String(c)][inv->Statistics()->getChannelFieldName(t, c, FLD_IRR)]["max"] = inv->Statistics()->getStringMaxPower(c);
                }
            }
        }

        if (inv->Statistics()->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
            invObject["events"] = inv->EventLog()->getEntryCount();
        } else {
            invObject["events"] = -1;
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
    if (!strcmp(Configuration.get().Security.Password, ACCESS_POINT_PASSWORD)) {
        hintObj["default_password"] = true;
    } else {
        hintObj["default_password"] = false;
    }
}

void WebApiWsLiveClass::addField(JsonObject& root, std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId, String topic)
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

void WebApiWsLiveClass::addTotalField(JsonObject& root, const String& name, const float value, const String& unit, const uint8_t digits)
{
    root[name]["v"] = value;
    root[name]["u"] = unit;
    root[name]["d"] = digits;
}

void WebApiWsLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] connect\r\n", server->url(), client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        MessageOutput.printf("Websocket: [%s][%u] disconnect\r\n", server->url(), client->id());
    }
}

void WebApiWsLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096 * INV_MAX_COUNT);
        auto& root = response->getRoot();

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

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ws_vedirect_live.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "WebApi.h"
#include "defaults.h"
#include "PowerLimiter.h"
#include "VictronMppt.h"

WebApiWsVedirectLiveClass::WebApiWsVedirectLiveClass()
    : _ws("/vedirectlivedata")
{
}

void WebApiWsVedirectLiveClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;
    _server->on("/api/vedirectlivedata/status", HTTP_GET, std::bind(&WebApiWsVedirectLiveClass::onLivedataStatus, this, _1));

    _server->addHandler(&_ws);
    _ws.onEvent(std::bind(&WebApiWsVedirectLiveClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));


    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.setCallback(std::bind(&WebApiWsVedirectLiveClass::wsCleanupTaskCb, this));
    _wsCleanupTask.setIterations(TASK_FOREVER);
    _wsCleanupTask.setInterval(1 * TASK_SECOND);
    _wsCleanupTask.enable();

    scheduler.addTask(_sendDataTask);
    _sendDataTask.setCallback(std::bind(&WebApiWsVedirectLiveClass::sendDataTaskCb, this));
    _sendDataTask.setIterations(TASK_FOREVER);
    _sendDataTask.setInterval(500 * TASK_MILLISECOND);
    _sendDataTask.enable();
}

void WebApiWsVedirectLiveClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

bool WebApiWsVedirectLiveClass::hasUpdate(size_t idx)
{
    auto dataAgeMillis = VictronMppt.getDataAgeMillis(idx);
    if (dataAgeMillis == 0) { return false; }
    auto publishAgeMillis = millis() - _lastPublish;
    return dataAgeMillis < publishAgeMillis;
}

uint16_t WebApiWsVedirectLiveClass::responseSize() const
{
    // estimated with ArduinoJson assistant
    return VictronMppt.controllerAmount() * (1024 + 512) + 128/*DPL status and structure*/;
}

void WebApiWsVedirectLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) { return; }

    // Update on ve.direct change or at least after 10 seconds
    bool fullUpdate = (millis() - _lastFullPublish > (10 * 1000));
    bool updateAvailable = false;
    if (!fullUpdate) {
        for (size_t idx = 0; idx < VictronMppt.controllerAmount(); ++idx) {
            if (hasUpdate(idx)) {
                updateAvailable = true;
                break;
            }
        }
    }

    if (fullUpdate || updateAvailable) {
        try {
            std::lock_guard<std::mutex> lock(_mutex);
            DynamicJsonDocument root(responseSize());
            if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
                JsonVariant var = root;
                generateJsonResponse(var, fullUpdate);

                if (Utils::checkJsonOverflow(root, __FUNCTION__, __LINE__)) { return; }

                String buffer;
                serializeJson(root, buffer);

                if (Configuration.get().Security.AllowReadonly) {
                    _ws.setAuthentication("", "");
                } else {
                    _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security.Password);
                }

                _ws.textAll(buffer);
            }

        } catch (std::bad_alloc& bad_alloc) {
            MessageOutput.printf("Calling /api/vedirectlivedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        } catch (const std::exception& exc) {
            MessageOutput.printf("Unknown exception in /api/vedirectlivedata/status. Reason: \"%s\".\r\n", exc.what());
        }
    }

    if (fullUpdate) {
        _lastFullPublish = millis();
    }
}

void WebApiWsVedirectLiveClass::generateJsonResponse(JsonVariant& root, bool fullUpdate)
{
    const JsonObject &array = root["vedirect"].createNestedObject("instances");
    root["vedirect"]["full_update"] = fullUpdate;

    for (size_t idx = 0; idx < VictronMppt.controllerAmount(); ++idx) {
        auto optMpptData = VictronMppt.getData(idx);
        if (!optMpptData.has_value()) { continue; }

        if (!fullUpdate && !hasUpdate(idx)) { continue; }

        String serial(optMpptData->SER);
        if (serial.isEmpty()) { continue; } // serial required as index

        const JsonObject &nested = array.createNestedObject(serial);
        nested["data_age_ms"] = VictronMppt.getDataAgeMillis(idx);
        populateJson(nested, *optMpptData);
    }

    _lastPublish = millis();

    // power limiter state
    root["dpl"]["PLSTATE"] = -1;
    if (Configuration.get().PowerLimiter.Enabled)
        root["dpl"]["PLSTATE"] = PowerLimiter.getPowerLimiterState();
    root["dpl"]["PLLIMIT"] = PowerLimiter.getLastRequestedPowerLimit();
}

void WebApiWsVedirectLiveClass::populateJson(const JsonObject &root, const VeDirectMpptController::data_t &mpptData) {
    root["product_id"] = mpptData.getPidAsString();
    root["firmware_version"] = String(mpptData.FW);

    const JsonObject &values = root.createNestedObject("values");

    const JsonObject &device = values.createNestedObject("device");
    device["LOAD"] = mpptData.LOAD ? "ON" : "OFF";
    device["CS"] = mpptData.getCsAsString();
    device["MPPT"] = mpptData.getMpptAsString();
    device["OR"] = mpptData.getOrAsString();
    device["ERR"] = mpptData.getErrAsString();
    device["HSDS"]["v"] = mpptData.HSDS;
    device["HSDS"]["u"] = "d";
    if (mpptData.MpptTemperatureMilliCelsius.first > 0) {
        device["MpptTemperature"]["v"] = mpptData.MpptTemperatureMilliCelsius.second / 1000.0;
        device["MpptTemperature"]["u"] = "°C";
        device["MpptTemperature"]["d"] = "1";
    }

    const JsonObject &output = values.createNestedObject("output");
    output["P"]["v"] = mpptData.P;
    output["P"]["u"] = "W";
    output["P"]["d"] = 0;
    output["V"]["v"] = mpptData.V;
    output["V"]["u"] = "V";
    output["V"]["d"] = 2;
    output["I"]["v"] = mpptData.I;
    output["I"]["u"] = "A";
    output["I"]["d"] = 2;
    output["E"]["v"] = mpptData.E;
    output["E"]["u"] = "%";
    output["E"]["d"] = 1;

    const JsonObject &input = values.createNestedObject("input");
    if (mpptData.NetworkTotalDcInputPowerMilliWatts.first > 0) {
        input["NetworkPower"]["v"] = mpptData.NetworkTotalDcInputPowerMilliWatts.second / 1000.0;
        input["NetworkPower"]["u"] = "W";
        input["NetworkPower"]["d"] = "0";
    }
    input["PPV"]["v"] = mpptData.PPV;
    input["PPV"]["u"] = "W";
    input["PPV"]["d"] = 0;
    input["VPV"]["v"] = mpptData.VPV;
    input["VPV"]["u"] = "V";
    input["VPV"]["d"] = 2;
    input["IPV"]["v"] = mpptData.IPV;
    input["IPV"]["u"] = "A";
    input["IPV"]["d"] = 2;
    input["YieldToday"]["v"] = mpptData.H20;
    input["YieldToday"]["u"] = "kWh";
    input["YieldToday"]["d"] = 3;
    input["YieldYesterday"]["v"] = mpptData.H22;
    input["YieldYesterday"]["u"] = "kWh";
    input["YieldYesterday"]["d"] = 3;
    input["YieldTotal"]["v"] = mpptData.H19;
    input["YieldTotal"]["u"] = "kWh";
    input["YieldTotal"]["d"] = 3;
    input["MaximumPowerToday"]["v"] = mpptData.H21;
    input["MaximumPowerToday"]["u"] = "W";
    input["MaximumPowerToday"]["d"] = 0;
    input["MaximumPowerYesterday"]["v"] = mpptData.H23;
    input["MaximumPowerYesterday"]["u"] = "W";
    input["MaximumPowerYesterday"]["d"] = 0;
}

void WebApiWsVedirectLiveClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
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

void WebApiWsVedirectLiveClass::onLivedataStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    try {
        std::lock_guard<std::mutex> lock(_mutex);
        AsyncJsonResponse* response = new AsyncJsonResponse(false, responseSize());
        auto& root = response->getRoot();

        generateJsonResponse(root, true/*fullUpdate*/);

        response->setLength();
        request->send(response);

    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Calling /api/vedirectlivedata/status has temporarily run out of resources. Reason: \"%s\".\r\n", bad_alloc.what());
        WebApi.sendTooManyRequests(request);
    } catch (const std::exception& exc) {
        MessageOutput.printf("Unknown exception in /api/vedirectlivedata/status. Reason: \"%s\".\r\n", exc.what());
        WebApi.sendTooManyRequests(request);
    }
}

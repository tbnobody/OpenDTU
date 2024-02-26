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

void WebApiWsVedirectLiveClass::sendDataTaskCb()
{
    // do nothing if no WS client is connected
    if (_ws.count() == 0) {
        return;
    }

    // we assume this loop to be running at least twice for every
    // update from a VE.Direct MPPT data producer, so _dataAgeMillis
    // actually grows in between updates.
    auto lastDataAgeMillis = _dataAgeMillis;
    _dataAgeMillis = VictronMppt.getDataAgeMillis();

    // Update on ve.direct change or at least after 10 seconds
    if (millis() - _lastWsPublish > (10 * 1000) || lastDataAgeMillis > _dataAgeMillis) {

        try {
            std::lock_guard<std::mutex> lock(_mutex);
            DynamicJsonDocument root(_responseSize);
            if (Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
                JsonVariant var = root;
                generateJsonResponse(var);

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

        _lastWsPublish = millis();
    }
}

void WebApiWsVedirectLiveClass::generateJsonResponse(JsonVariant& root)
{
    root["vedirect"]["data_age"] = VictronMppt.getDataAgeMillis() / 1000;
    const JsonArray &array = root["vedirect"].createNestedArray("devices");

    for (int idx = 0; idx < VICTRON_MAX_COUNT; ++idx) {
        std::optional<VeDirectMpptController::spData_t> spOptMpptData = VictronMppt.getData(idx);
        if (!spOptMpptData.has_value()) {
            continue;
        }

        VeDirectMpptController::spData_t &spMpptData = spOptMpptData.value();

        const JsonObject &nested = array.createNestedObject();
        nested["age_critical"] = !VictronMppt.isDataValid(idx);
        populateJson(nested, spMpptData);
    }

    // power limiter state
    root["dpl"]["PLSTATE"] = -1;
    if (Configuration.get().PowerLimiter.Enabled)
        root["dpl"]["PLSTATE"] = PowerLimiter.getPowerLimiterState();
    root["dpl"]["PLLIMIT"] = PowerLimiter.getLastRequestedPowerLimit();
}

void
WebApiWsVedirectLiveClass::populateJson(const JsonObject &root, const VeDirectMpptController::spData_t &spMpptData) {
    // device info
    root["device"]["PID"] = spMpptData->getPidAsString();
    root["device"]["SER"] = spMpptData->SER;
    root["device"]["FW"] = spMpptData->FW;
    root["device"]["LOAD"] = spMpptData->LOAD ? "ON" : "OFF";
    root["device"]["CS"] = spMpptData->getCsAsString();
    root["device"]["ERR"] = spMpptData->getErrAsString();
    root["device"]["OR"] = spMpptData->getOrAsString();
    root["device"]["MPPT"] = spMpptData->getMpptAsString();
    root["device"]["HSDS"]["v"] = spMpptData->HSDS;
    root["device"]["HSDS"]["u"] = "d";

    // battery info
    root["output"]["P"]["v"] = spMpptData->P;
    root["output"]["P"]["u"] = "W";
    root["output"]["P"]["d"] = 0;
    root["output"]["V"]["v"] = spMpptData->V;
    root["output"]["V"]["u"] = "V";
    root["output"]["V"]["d"] = 2;
    root["output"]["I"]["v"] = spMpptData->I;
    root["output"]["I"]["u"] = "A";
    root["output"]["I"]["d"] = 2;
    root["output"]["E"]["v"] = spMpptData->E;
    root["output"]["E"]["u"] = "%";
    root["output"]["E"]["d"] = 1;

    // panel info
    root["input"]["PPV"]["v"] = spMpptData->PPV;
    root["input"]["PPV"]["u"] = "W";
    root["input"]["PPV"]["d"] = 0;
    root["input"]["VPV"]["v"] = spMpptData->VPV;
    root["input"]["VPV"]["u"] = "V";
    root["input"]["VPV"]["d"] = 2;
    root["input"]["IPV"]["v"] = spMpptData->IPV;
    root["input"]["IPV"]["u"] = "A";
    root["input"]["IPV"]["d"] = 2;
    root["input"]["YieldToday"]["v"] = spMpptData->H20;
    root["input"]["YieldToday"]["u"] = "kWh";
    root["input"]["YieldToday"]["d"] = 3;
    root["input"]["YieldYesterday"]["v"] = spMpptData->H22;
    root["input"]["YieldYesterday"]["u"] = "kWh";
    root["input"]["YieldYesterday"]["d"] = 3;
    root["input"]["YieldTotal"]["v"] = spMpptData->H19;
    root["input"]["YieldTotal"]["u"] = "kWh";
    root["input"]["YieldTotal"]["d"] = 3;
    root["input"]["MaximumPowerToday"]["v"] = spMpptData->H21;
    root["input"]["MaximumPowerToday"]["u"] = "W";
    root["input"]["MaximumPowerToday"]["d"] = 0;
    root["input"]["MaximumPowerYesterday"]["v"] = spMpptData->H23;
    root["input"]["MaximumPowerYesterday"]["u"] = "W";
    root["input"]["MaximumPowerYesterday"]["d"] = 0;
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
        AsyncJsonResponse* response = new AsyncJsonResponse(false, _responseSize);
        auto& root = response->getRoot();

        generateJsonResponse(root);

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

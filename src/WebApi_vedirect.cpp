// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_vedirect.h"
#include "VictronMppt.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include "MqttHandlePowerLimiterHass.h"

void WebApiVedirectClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/vedirect/status", HTTP_GET, std::bind(&WebApiVedirectClass::onVedirectStatus, this, _1));
    _server->on("/api/vedirect/config", HTTP_GET, std::bind(&WebApiVedirectClass::onVedirectAdminGet, this, _1));
    _server->on("/api/vedirect/config", HTTP_POST, std::bind(&WebApiVedirectClass::onVedirectAdminPost, this, _1));
}

void WebApiVedirectClass::onVedirectStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["vedirect_enabled"] = config.Vedirect.Enabled;
    root["verbose_logging"] = config.Vedirect.VerboseLogging;
    root["vedirect_updatesonly"] = config.Vedirect.UpdatesOnly;

    response->setLength();
    request->send(response);
}

void WebApiVedirectClass::onVedirectAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["vedirect_enabled"] = config.Vedirect.Enabled;
    root["verbose_logging"] = config.Vedirect.VerboseLogging;
    root["vedirect_updatesonly"] = config.Vedirect.UpdatesOnly;

    response->setLength();
    request->send(response);
}

void WebApiVedirectClass::onVedirectAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!root.containsKey("vedirect_enabled") ||
            !root.containsKey("verbose_logging") ||
            !root.containsKey("vedirect_updatesonly") ) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.Vedirect.Enabled = root["vedirect_enabled"].as<bool>();
    config.Vedirect.VerboseLogging = root["verbose_logging"].as<bool>();
    config.Vedirect.UpdatesOnly = root["vedirect_updatesonly"].as<bool>();

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);


    VictronMppt.updateSettings();

    // potentially make solar passthrough thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

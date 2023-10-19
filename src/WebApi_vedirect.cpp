// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_vedirect.h"
#include "VictronMppt.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"

void WebApiVedirectClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/vedirect/status", HTTP_GET, std::bind(&WebApiVedirectClass::onVedirectStatus, this, _1));
    _server->on("/api/vedirect/config", HTTP_GET, std::bind(&WebApiVedirectClass::onVedirectAdminGet, this, _1));
    _server->on("/api/vedirect/config", HTTP_POST, std::bind(&WebApiVedirectClass::onVedirectAdminPost, this, _1));
}

void WebApiVedirectClass::loop()
{
}

void WebApiVedirectClass::onVedirectStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("vedirect_enabled")] = config.Vedirect_Enabled;
    root[F("verbose_logging")] = config.Vedirect_VerboseLogging;
    root[F("vedirect_updatesonly")] = config.Vedirect_UpdatesOnly;

    response->setLength();
    request->send(response);
}

void WebApiVedirectClass::onVedirectAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("vedirect_enabled")] = config.Vedirect_Enabled;
    root[F("verbose_logging")] = config.Vedirect_VerboseLogging;
    root[F("vedirect_updatesonly")] = config.Vedirect_UpdatesOnly;

    response->setLength();
    request->send(response);
}

void WebApiVedirectClass::onVedirectAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!root.containsKey("vedirect_enabled") ||
            !root.containsKey("verbose_logging") ||
            !root.containsKey("vedirect_updatesonly") ) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.Vedirect_Enabled = root[F("vedirect_enabled")].as<bool>();
    config.Vedirect_VerboseLogging = root[F("verbose_logging")].as<bool>();
    config.Vedirect_UpdatesOnly = root[F("vedirect_updatesonly")].as<bool>();
    Configuration.write();

    VictronMppt.init();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}

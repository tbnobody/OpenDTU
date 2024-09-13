// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Battery.h"
#include "Configuration.h"
#include "MqttHandleBatteryHass.h"
#include "MqttHandlePowerLimiterHass.h"
#include "WebApi.h"
#include "WebApi_battery.h"
#include "WebApi_errors.h"
#include "helper.h"

void WebApiBatteryClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/battery/status", HTTP_GET, std::bind(&WebApiBatteryClass::onStatus, this, _1));
    _server->on("/api/battery/config", HTTP_GET, std::bind(&WebApiBatteryClass::onAdminGet, this, _1));
    _server->on("/api/battery/config", HTTP_POST, std::bind(&WebApiBatteryClass::onAdminPost, this, _1));
}

void WebApiBatteryClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto& config = Configuration.get();

    ConfigurationClass::serializeBatteryConfig(config.Battery, root);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiBatteryClass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    onStatus(request);
}

void WebApiBatteryClass::onAdminPost(AsyncWebServerRequest* request)
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

    if (!root.containsKey("enabled") || !root.containsKey("provider")) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto& config = Configuration.get();
    ConfigurationClass::deserializeBatteryConfig(root.as<JsonObject>(), config.Battery);

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    Battery.updateSettings();
    MqttHandleBatteryHass.forceUpdate();

    // potentially make SoC thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

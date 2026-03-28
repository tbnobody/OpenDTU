// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

#include "ArduinoJson.h"
#include "AsyncJson.h"
#include <battery/Controller.h>
#include "Configuration.h"
#include "MqttHandlePowerLimiterHass.h"
#include "WebApi.h"
#include "WebApi_battery.h"
#include "WebApi_errors.h"
#include "helper.h"

void WebApiBatteryClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/battery/status", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiBatteryClass::onStatus, this, _1)));
    _server->on("/api/battery/config", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiBatteryClass::onAdminGet, this, _1)));
    _server->on("/api/battery/config", HTTP_POST, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiBatteryClass::onAdminPost, this, _1)));
}

void WebApiBatteryClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto const& config = Configuration.get();

    ConfigurationClass::serializeBatteryConfig(config.Battery, root);

    auto zendure = root["zendure"].to<JsonObject>();
    ConfigurationClass::serializeBatteryZendureConfig(config.Battery.Zendure, zendure);

    auto mqtt = root["mqtt"].to<JsonObject>();
    ConfigurationClass::serializeBatteryMqttConfig(config.Battery.Mqtt, mqtt);

    auto serial = root["serial"].to<JsonObject>();
    ConfigurationClass::serializeBatterySerialConfig(config.Battery.Serial, serial);

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

    if (!root["enabled"].is<bool>() || !root["provider"].is<uint8_t>()) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();
        ConfigurationClass::deserializeBatteryConfig(root.as<JsonObject>(), config.Battery);
        ConfigurationClass::deserializeBatteryZendureConfig(root["zendure"].as<JsonObject>(), config.Battery.Zendure);
        ConfigurationClass::deserializeBatteryMqttConfig(root["mqtt"].as<JsonObject>(), config.Battery.Mqtt);
        ConfigurationClass::deserializeBatterySerialConfig(root["serial"].as<JsonObject>(), config.Battery.Serial);
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    Battery.updateSettings();

    // potentially make SoC thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

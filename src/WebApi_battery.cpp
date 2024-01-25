// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Battery.h"
#include "Configuration.h"
#include "PylontechCanReceiver.h"
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
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["enabled"] = config.Battery.Enabled;
    root["verbose_logging"] = config.Battery.VerboseLogging;
    root["provider"] = config.Battery.Provider;
    root["jkbms_interface"] = config.Battery.JkBmsInterface;
    root["jkbms_polling_interval"] = config.Battery.JkBmsPollingInterval;
    root["mqtt_topic"] = config.Battery.MqttTopic;

    response->setLength();
    request->send(response);
}

void WebApiBatteryClass::onAdminGet(AsyncWebServerRequest* request)
{
    onStatus(request);
}

void WebApiBatteryClass::onAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!root.containsKey("enabled") || !root.containsKey("provider")) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.Battery.Enabled = root["enabled"].as<bool>();
    config.Battery.VerboseLogging = root["verbose_logging"].as<bool>();
    config.Battery.Provider = root["provider"].as<uint8_t>();
    config.Battery.JkBmsInterface = root["jkbms_interface"].as<uint8_t>();
    config.Battery.JkBmsPollingInterval = root["jkbms_polling_interval"].as<uint8_t>();
    strlcpy(config.Battery.MqttTopic, root["mqtt_topic"].as<String>().c_str(), sizeof(config.Battery.MqttTopic));
    
    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);

    Battery.updateSettings();
}

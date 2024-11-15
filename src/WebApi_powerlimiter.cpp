// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_powerlimiter.h"
#include "VeDirectFrameHandler.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MqttHandlePowerLimiterHass.h"
#include "PowerLimiter.h"
#include "WebApi.h"
#include "helper.h"
#include "WebApi_errors.h"
#include "Configuration.h"

void WebApiPowerLimiterClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/powerlimiter/status", HTTP_GET, std::bind(&WebApiPowerLimiterClass::onStatus, this, _1));
    _server->on("/api/powerlimiter/config", HTTP_GET, std::bind(&WebApiPowerLimiterClass::onAdminGet, this, _1));
    _server->on("/api/powerlimiter/config", HTTP_POST, std::bind(&WebApiPowerLimiterClass::onAdminPost, this, _1));
    _server->on("/api/powerlimiter/metadata", HTTP_GET, std::bind(&WebApiPowerLimiterClass::onMetaData, this, _1));
}

void WebApiPowerLimiterClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto const& config = Configuration.get();
    ConfigurationClass::serializePowerLimiterConfig(config.PowerLimiter, root);
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiPowerLimiterClass::onMetaData(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) { return; }

    auto const& config = Configuration.get();

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    root["power_meter_enabled"] = config.PowerMeter.Enabled;
    root["battery_enabled"] = config.Battery.Enabled;
    root["charge_controller_enabled"] = config.Vedirect.Enabled;

    JsonArray inverters = root["inverters"].to<JsonArray>();
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        auto inv = Hoymiles.getInverterBySerial(config.Inverter[i].Serial);
        if (!inv) { continue; }

        JsonObject obj = inverters.add<JsonObject>();
        obj["serial"] = inv->serialString();
        obj["pos"] = i;
        obj["name"] = String(config.Inverter[i].Name);
        obj["poll_enable"] = config.Inverter[i].Poll_Enable;
        obj["poll_enable_night"] = config.Inverter[i].Poll_Enable_Night;
        obj["command_enable"] = config.Inverter[i].Command_Enable;
        obj["command_enable_night"] = config.Inverter[i].Command_Enable_Night;
        obj["max_power"] = inv->DevInfo()->getMaxPower(); // okay if zero/unknown
        obj["type"] = inv->typeName();
        auto channels = inv->Statistics()->getChannelsByType(TYPE_DC);
        obj["channels"] = channels.size();
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiPowerLimiterClass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    this->onStatus(request);
}

void WebApiPowerLimiterClass::onAdminPost(AsyncWebServerRequest* request)
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

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();
        ConfigurationClass::deserializePowerLimiterConfig(root.as<JsonObject>(), config.PowerLimiter);
    }

    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);

    PowerLimiter.triggerReloadingConfig();

    // potentially make thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "WebApi_zeroexport.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "ZeroExport.h"
#include "defaults.h"
#include "helper.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiZeroExportClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/zeroexport/status", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiZeroExportClass::onZeroExportStatus, this, _1)));
    server.on("/api/zeroexport/config", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiZeroExportClass::onZeroExportAdminGet, this, _1)));
    server.on("/api/zeroexport/config", HTTP_POST, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiZeroExportClass::onZeroExportAdminPost, this, _1)));
}

void WebApiZeroExportClass::onZeroExportStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    root["enabled"] = ZeroExport.isEnabled();
    root["setpoint"] = ZeroExport.getSetPoint();
    root["grid_source_running"] = ZeroExport.isSourceRunning();
    const std::optional<float> gridPower = ZeroExport.getGridPower();
    if (gridPower.has_value()) {
        root["grid_power"] = *gridPower;
    } else {
        root["grid_power"] = nullptr;
    }
    root["grid_power_age"] = ZeroExport.getGridPowerAgeMs() / 1000UL;
    // Effective freshness timeout of the active grid source, in seconds.
    // Exposed next to grid_power_age so the two can be compared without a
    // round-trip to the config API: grid_power_age > grid_power_failsafe
    // means the sample has expired and the fail-safe applies.
    root["grid_power_failsafe"] = ZeroExport.getGridPowerFailsafeMs() / 1000UL;
    root["grid_update_period"] = ZeroExport.getSourceUpdatePeriodMs();
    root["production_limit"] = ZeroExport.getProductionLimit();
    root["failsafe"] = ZeroExport.isGridFailSafeActive();

    // Per-inverter states, aligned with the configured inverters. The serial
    // comes from the inverter object (never zero), so a slot whose state has
    // not been reconciled yet still shows its real serial with its pending
    // startup state. A position without a matching inverter (the state vector
    // is resized only by the Zero-Export loop, so it can lag behind an
    // inverter added or removed through the inverter settings) is not
    // exposed at all.
    auto inverters = root["inverters"].to<JsonArray>();
    const auto& states = ZeroExport.getInverterStates();
    for (uint8_t i = 0; i < states.size(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }
        // Inverter serial is formatted as HEX, like the inverter list API.
        char buffer[sizeof(uint64_t) * 8 + 1];
        snprintf(buffer, sizeof(buffer), "%0" PRIx32 "%08" PRIx32,
            static_cast<uint32_t>((inv->serial() >> 32) & 0xFFFFFFFF),
            static_cast<uint32_t>(inv->serial() & 0xFFFFFFFF));
        auto obj = inverters.add<JsonObject>();
        obj["pos"] = i;
        obj["serial"] = buffer;
        obj["name"] = inv->name();
        obj["limit"] = states[i].Limit;
        obj["startup_pending"] = states[i].StartupPending;
        obj["startup_at_full_power"] = states[i].StartupAtFullPower;
        obj["release_pending"] = states[i].ReleasePending;
        obj["released"] = states[i].Released;
        obj["failsafe_pending"] = states[i].FailSafePending;
        obj["reachable"] = inv->isReachable();
        obj["commands_enabled"] = inv->getEnableCommands();
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiZeroExportClass::onZeroExportAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["enabled"] = config.ZeroExport.Enabled;
    root["setpoint"] = config.ZeroExport.SetPoint;
    root["inverter_minimal_power"] = config.ZeroExport.InverterMinimalPower;
    root["home_minimal_consumption"] = config.ZeroExport.HomeMinimalConsumption;
    root["update_interval"] = config.ZeroExport.UpdateInterval;
    root["source"] = config.ZeroExport.Source;
    root["shelly_lnm_addr"] = config.ZeroExport.ShellyLnm.GroupAddress;
    root["shelly_lnm_port"] = config.ZeroExport.ShellyLnm.GroupPort;
    root["shelly_lnm_type"] = config.ZeroExport.ShellyLnm.Type;
    root["shelly_lnm_failsafe_timeout"] = config.ZeroExport.ShellyLnm.FailsafeTimeout;
    root["mqtt_grid_power_topic"] = config.ZeroExport.Mqtt.GridPowerTopic;
    root["mqtt_data_type"] = config.ZeroExport.Mqtt.Type;
    root["mqtt_failsafe_timeout"] = config.ZeroExport.Mqtt.FailsafeTimeout;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiZeroExportClass::onZeroExportAdminPost(AsyncWebServerRequest* request)
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

        if (!(root["enabled"].is<bool>()
            && root["setpoint"].is<int16_t>()
            && root["inverter_minimal_power"].is<uint16_t>()
            && root["home_minimal_consumption"].is<uint16_t>()
            && root["update_interval"].is<uint8_t>()
            && root["source"].is<uint8_t>()
            && root["shelly_lnm_addr"].is<String>()
            && root["shelly_lnm_port"].is<uint16_t>()
            && root["shelly_lnm_type"].is<String>()
            && root["shelly_lnm_failsafe_timeout"].is<uint16_t>()
            && root["mqtt_grid_power_topic"].is<String>()
            && root["mqtt_data_type"].is<String>()
            && root["mqtt_failsafe_timeout"].is<uint16_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["source"].as<uint8_t>() == ZEROEXPORT_SOURCE_MQTT
        && (root["mqtt_grid_power_topic"].as<String>().length() == 0
            || root["mqtt_grid_power_topic"].as<String>().length() > ZEROEXPORT_MAX_MQTT_TOPIC_STRLEN)) {
        retMsg["message"] = "Grid power topic must between 1 and " STR_EXTRACT(ZEROEXPORT_MAX_MQTT_TOPIC_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::ZeroExportTopicLength;
        retMsg["param"]["max"] = ZEROEXPORT_MAX_MQTT_TOPIC_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["shelly_lnm_addr"].as<String>().length() == 0
        || root["shelly_lnm_addr"].as<String>().length() > ZEROEXPORT_MAX_ADDR_STRLEN) {
        retMsg["message"] = "Shelly LNM address must between 1 and " STR_EXTRACT(ZEROEXPORT_MAX_ADDR_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::ZeroExportAddrLength;
        retMsg["param"]["max"] = ZEROEXPORT_MAX_ADDR_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const String shellyLnmType = root["shelly_lnm_type"].as<String>();
    if (!(shellyLnmType == "act_power"
        || shellyLnmType == "total_act_power"
        || shellyLnmType == "a_act_power"
        || shellyLnmType == "b_act_power"
        || shellyLnmType == "c_act_power")) {
        retMsg["message"] = "Shelly LNM data type is invalid!";
        retMsg["code"] = WebApiError::ZeroExportInvalidShellyLnmType;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const uint8_t updateInterval = root["update_interval"].as<uint8_t>();
    if (updateInterval < ZEROEXPORT_UPDATE_INTERVAL_MIN) {
        retMsg["message"] = "Update interval must be a number between " STR_EXTRACT(ZEROEXPORT_UPDATE_INTERVAL_MIN) " and 255!";
        retMsg["code"] = WebApiError::ZeroExportInvalidUpdateInterval;
        retMsg["param"]["min"] = ZEROEXPORT_UPDATE_INTERVAL_MIN;
        retMsg["param"]["max"] = 255;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const uint16_t inverterMinimalPower = root["inverter_minimal_power"].as<uint16_t>();
    if (inverterMinimalPower < ZEROEXPORT_INVERTER_MINIMAL_POWER) {
        retMsg["message"] = "Minimal production per inverter must be at least " STR_EXTRACT(ZEROEXPORT_INVERTER_MINIMAL_POWER) " watts!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        retMsg["param"]["min"] = ZEROEXPORT_INVERTER_MINIMAL_POWER;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const uint16_t port = root["shelly_lnm_port"].as<uint16_t>();
    if (port == 0) {
        retMsg["message"] = "Port must be a number between 1 and 65535!";
        retMsg["code"] = WebApiError::ZeroExportInvalidPort;
        retMsg["param"]["min"] = 1;
        retMsg["param"]["max"] = 65535;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const uint16_t shellyFailsafeTimeout = root["shelly_lnm_failsafe_timeout"].as<uint16_t>();
    if (shellyFailsafeTimeout < 5 || shellyFailsafeTimeout > 300) {
        retMsg["message"] = "Shelly LNM failsafe timeout must be a number between 5 and 300 seconds!";
        retMsg["code"] = WebApiError::ZeroExportInvalidUpdateInterval;
        retMsg["param"]["min"] = 5;
        retMsg["param"]["max"] = 300;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const String mqttDataType = root["mqtt_data_type"].as<String>();
    if (!(mqttDataType == "raw"
        || mqttDataType == "act_power"
        || mqttDataType == "total_act_power"
        || mqttDataType == "a_act_power"
        || mqttDataType == "b_act_power"
        || mqttDataType == "c_act_power")) {
        retMsg["message"] = "MQTT data type is invalid!";
        retMsg["code"] = WebApiError::ZeroExportInvalidMqttDataType;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    const uint16_t mqttFailsafeTimeout = root["mqtt_failsafe_timeout"].as<uint16_t>();
    if (mqttFailsafeTimeout < 5 || mqttFailsafeTimeout > 300) {
        retMsg["message"] = "MQTT failsafe timeout must be a number between 5 and 300 seconds!";
        retMsg["code"] = WebApiError::ZeroExportInvalidUpdateInterval;
        retMsg["param"]["min"] = 5;
        retMsg["param"]["max"] = 300;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        config.ZeroExport.Enabled = root["enabled"].as<bool>();
        config.ZeroExport.SetPoint = root["setpoint"].as<int16_t>();
        config.ZeroExport.InverterMinimalPower = inverterMinimalPower;
        config.ZeroExport.HomeMinimalConsumption = root["home_minimal_consumption"].as<uint16_t>();
        config.ZeroExport.UpdateInterval = updateInterval;
        config.ZeroExport.Source = root["source"].as<uint8_t>();
        strlcpy(config.ZeroExport.ShellyLnm.GroupAddress, root["shelly_lnm_addr"].as<String>().c_str(), sizeof(config.ZeroExport.ShellyLnm.GroupAddress));
        config.ZeroExport.ShellyLnm.GroupPort = port;
        strlcpy(config.ZeroExport.ShellyLnm.Type, root["shelly_lnm_type"].as<String>().c_str(), sizeof(config.ZeroExport.ShellyLnm.Type));
        config.ZeroExport.ShellyLnm.FailsafeTimeout = shellyFailsafeTimeout;
        strlcpy(config.ZeroExport.Mqtt.GridPowerTopic, root["mqtt_grid_power_topic"].as<String>().c_str(), sizeof(config.ZeroExport.Mqtt.GridPowerTopic));
        strlcpy(config.ZeroExport.Mqtt.Type, mqttDataType.c_str(), sizeof(config.ZeroExport.Mqtt.Type));
        config.ZeroExport.Mqtt.FailsafeTimeout = mqttFailsafeTimeout;
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    // applyConfig() mutates Zero-Export state (_invStates, _wasEnabled, ...)
    // that the regulation loop uses without locks in the main-loop task, so
    // it must run there too. The request flag is consumed by the Zero-Export
    // loop on its next 500 ms pass.
    ZeroExport.requestApplyConfig();
    MqttHandleHass.forceUpdate();
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_powerlimiter.h"
#include "VeDirectFrameHandler.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "MqttHandleVedirectHass.h"
#include "MqttSettings.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "WebApi.h"
#include "helper.h"
#include "WebApi_errors.h"

void WebApiPowerLimiterClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/powerlimiter/status", HTTP_GET, std::bind(&WebApiPowerLimiterClass::onStatus, this, _1));
    _server->on("/api/powerlimiter/config", HTTP_GET, std::bind(&WebApiPowerLimiterClass::onAdminGet, this, _1));
    _server->on("/api/powerlimiter/config", HTTP_POST, std::bind(&WebApiPowerLimiterClass::onAdminPost, this, _1));
}

void WebApiPowerLimiterClass::loop()
{
}

void WebApiPowerLimiterClass::onStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["enabled"] = config.PowerLimiter.Enabled;
    root["verbose_logging"] = config.PowerLimiter.VerboseLogging;
    root["solar_passthrough_enabled"] = config.PowerLimiter.SolarPassThroughEnabled;
    root["solar_passthrough_losses"] = config.PowerLimiter.SolarPassThroughLosses;
    root["battery_drain_strategy"] = config.PowerLimiter.BatteryDrainStategy;
    root["is_inverter_behind_powermeter"] = config.PowerLimiter.IsInverterBehindPowerMeter;
    root["inverter_id"] = config.PowerLimiter.InverterId;
    root["inverter_channel_id"] = config.PowerLimiter.InverterChannelId;
    root["target_power_consumption"] = config.PowerLimiter.TargetPowerConsumption;
    root["target_power_consumption_hysteresis"] = config.PowerLimiter.TargetPowerConsumptionHysteresis;
    root["lower_power_limit"] = config.PowerLimiter.LowerPowerLimit;
    root["upper_power_limit"] = config.PowerLimiter.UpperPowerLimit;
    root["battery_soc_start_threshold"] = config.PowerLimiter.BatterySocStartThreshold;
    root["battery_soc_stop_threshold"] = config.PowerLimiter.BatterySocStopThreshold;
    root["voltage_start_threshold"] = static_cast<int>(config.PowerLimiter.VoltageStartThreshold * 100 +0.5) / 100.0;
    root["voltage_stop_threshold"] = static_cast<int>(config.PowerLimiter.VoltageStopThreshold * 100 +0.5) / 100.0;;
    root["voltage_load_correction_factor"] = config.PowerLimiter.VoltageLoadCorrectionFactor;
    root["inverter_restart_hour"] = config.PowerLimiter.RestartHour;
    root["full_solar_passthrough_soc"] = config.PowerLimiter.FullSolarPassThroughSoc;
    root["full_solar_passthrough_start_voltage"] = static_cast<int>(config.PowerLimiter.FullSolarPassThroughStartVoltage * 100 + 0.5) / 100.0;
    root["full_solar_passthrough_stop_voltage"] = static_cast<int>(config.PowerLimiter.FullSolarPassThroughStopVoltage * 100 + 0.5) / 100.0;

    response->setLength();
    request->send(response);
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
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("enabled") 
        	&& root.containsKey("lower_power_limit")
            && root.containsKey("inverter_id")
            && root.containsKey("inverter_channel_id")
            && root.containsKey("target_power_consumption")
            && root.containsKey("target_power_consumption_hysteresis")
        )) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }


    CONFIG_T& config = Configuration.get();
    config.PowerLimiter.Enabled = root["enabled"].as<bool>();
    PowerLimiter.setMode(PowerLimiterClass::Mode::Normal);  // User input sets PL to normal operation
    config.PowerLimiter.VerboseLogging = root["verbose_logging"].as<bool>();
    config.PowerLimiter.SolarPassThroughEnabled = root["solar_passthrough_enabled"].as<bool>();
    config.PowerLimiter.SolarPassThroughLosses = root["solar_passthrough_losses"].as<uint8_t>();
    config.PowerLimiter.BatteryDrainStategy= root["battery_drain_strategy"].as<uint8_t>();
    config.PowerLimiter.IsInverterBehindPowerMeter = root["is_inverter_behind_powermeter"].as<bool>();
    config.PowerLimiter.InverterId = root["inverter_id"].as<uint8_t>();
    config.PowerLimiter.InverterChannelId = root["inverter_channel_id"].as<uint8_t>();
    config.PowerLimiter.TargetPowerConsumption = root["target_power_consumption"].as<int32_t>();
    config.PowerLimiter.TargetPowerConsumptionHysteresis = root["target_power_consumption_hysteresis"].as<int32_t>();
    config.PowerLimiter.LowerPowerLimit = root["lower_power_limit"].as<int32_t>();
    config.PowerLimiter.UpperPowerLimit = root["upper_power_limit"].as<int32_t>();
    config.PowerLimiter.BatterySocStartThreshold = root["battery_soc_start_threshold"].as<uint32_t>();
    config.PowerLimiter.BatterySocStopThreshold = root["battery_soc_stop_threshold"].as<uint32_t>();
    config.PowerLimiter.VoltageStartThreshold = root["voltage_start_threshold"].as<float>();
    config.PowerLimiter.VoltageStartThreshold = static_cast<int>(config.PowerLimiter.VoltageStartThreshold * 100) / 100.0;
    config.PowerLimiter.VoltageStopThreshold = root["voltage_stop_threshold"].as<float>();
    config.PowerLimiter.VoltageStopThreshold = static_cast<int>(config.PowerLimiter.VoltageStopThreshold * 100) / 100.0;
    config.PowerLimiter.VoltageLoadCorrectionFactor = root["voltage_load_correction_factor"].as<float>();
    config.PowerLimiter.RestartHour = root["inverter_restart_hour"].as<int8_t>();
    config.PowerLimiter.FullSolarPassThroughSoc = root["full_solar_passthrough_soc"].as<uint32_t>();
    config.PowerLimiter.FullSolarPassThroughStartVoltage = static_cast<int>(root["full_solar_passthrough_start_voltage"].as<float>() * 100) / 100.0;
    config.PowerLimiter.FullSolarPassThroughStopVoltage = static_cast<int>(root["full_solar_passthrough_stop_voltage"].as<float>() * 100) / 100.0;

    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);

    PowerLimiter.calcNextInverterRestart();
}

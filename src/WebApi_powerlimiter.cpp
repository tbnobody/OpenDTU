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
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("enabled")] = config.PowerLimiter.Enabled;
    root[F("verbose_logging")] = config.PowerLimiter.VerboseLogging;
    root[F("solar_passthrough_enabled")] = config.PowerLimiter.SolarPassThroughEnabled;
    root[F("solar_passthrough_losses")] = config.PowerLimiter.SolarPassThroughLosses;
    root[F("battery_drain_strategy")] = config.PowerLimiter.BatteryDrainStategy;
    root[F("is_inverter_behind_powermeter")] = config.PowerLimiter.IsInverterBehindPowerMeter;
    root[F("inverter_id")] = config.PowerLimiter.InverterId;
    root[F("inverter_channel_id")] = config.PowerLimiter.InverterChannelId;
    root[F("target_power_consumption")] = config.PowerLimiter.TargetPowerConsumption;
    root[F("target_power_consumption_hysteresis")] = config.PowerLimiter.TargetPowerConsumptionHysteresis;
    root[F("lower_power_limit")] = config.PowerLimiter.LowerPowerLimit;
    root[F("upper_power_limit")] = config.PowerLimiter.UpperPowerLimit;
    root[F("battery_soc_start_threshold")] = config.PowerLimiter.BatterySocStartThreshold;
    root[F("battery_soc_stop_threshold")] = config.PowerLimiter.BatterySocStopThreshold;
    root[F("voltage_start_threshold")] = static_cast<int>(config.PowerLimiter.VoltageStartThreshold * 100 +0.5) / 100.0;
    root[F("voltage_stop_threshold")] = static_cast<int>(config.PowerLimiter.VoltageStopThreshold * 100 +0.5) / 100.0;;
    root[F("voltage_load_correction_factor")] = config.PowerLimiter.VoltageLoadCorrectionFactor;
    root[F("inverter_restart_hour")] = config.PowerLimiter.RestartHour;
    root[F("full_solar_passthrough_soc")] = config.PowerLimiter.FullSolarPassThroughSoc;
    root[F("full_solar_passthrough_start_voltage")] = static_cast<int>(config.PowerLimiter.FullSolarPassThroughStartVoltage * 100 + 0.5) / 100.0;
    root[F("full_solar_passthrough_stop_voltage")] = static_cast<int>(config.PowerLimiter.FullSolarPassThroughStopVoltage * 100 + 0.5) / 100.0;

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
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
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
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }


    CONFIG_T& config = Configuration.get();
    config.PowerLimiter.Enabled = root[F("enabled")].as<bool>();
    PowerLimiter.setMode(PowerLimiterClass::Mode::Normal);  // User input sets PL to normal operation
    config.PowerLimiter.VerboseLogging = root[F("verbose_logging")].as<bool>();
    config.PowerLimiter.SolarPassThroughEnabled = root[F("solar_passthrough_enabled")].as<bool>();
    config.PowerLimiter.SolarPassThroughLosses = root[F("solar_passthrough_losses")].as<uint8_t>();
    config.PowerLimiter.BatteryDrainStategy= root[F("battery_drain_strategy")].as<uint8_t>();
    config.PowerLimiter.IsInverterBehindPowerMeter = root[F("is_inverter_behind_powermeter")].as<bool>();
    config.PowerLimiter.InverterId = root[F("inverter_id")].as<uint8_t>();
    config.PowerLimiter.InverterChannelId = root[F("inverter_channel_id")].as<uint8_t>();
    config.PowerLimiter.TargetPowerConsumption = root[F("target_power_consumption")].as<int32_t>();
    config.PowerLimiter.TargetPowerConsumptionHysteresis = root[F("target_power_consumption_hysteresis")].as<int32_t>();
    config.PowerLimiter.LowerPowerLimit = root[F("lower_power_limit")].as<int32_t>();
    config.PowerLimiter.UpperPowerLimit = root[F("upper_power_limit")].as<int32_t>();
    config.PowerLimiter.BatterySocStartThreshold = root[F("battery_soc_start_threshold")].as<uint32_t>();
    config.PowerLimiter.BatterySocStopThreshold = root[F("battery_soc_stop_threshold")].as<uint32_t>();
    config.PowerLimiter.VoltageStartThreshold = root[F("voltage_start_threshold")].as<float>();
    config.PowerLimiter.VoltageStartThreshold = static_cast<int>(config.PowerLimiter.VoltageStartThreshold * 100) / 100.0;
    config.PowerLimiter.VoltageStopThreshold = root[F("voltage_stop_threshold")].as<float>();
    config.PowerLimiter.VoltageStopThreshold = static_cast<int>(config.PowerLimiter.VoltageStopThreshold * 100) / 100.0;
    config.PowerLimiter.VoltageLoadCorrectionFactor = root[F("voltage_load_correction_factor")].as<float>();
    config.PowerLimiter.RestartHour = root[F("inverter_restart_hour")].as<int8_t>();
    config.PowerLimiter.FullSolarPassThroughSoc = root[F("full_solar_passthrough_soc")].as<uint32_t>();
    config.PowerLimiter.FullSolarPassThroughStartVoltage = static_cast<int>(root[F("full_solar_passthrough_start_voltage")].as<float>() * 100) / 100.0;
    config.PowerLimiter.FullSolarPassThroughStopVoltage = static_cast<int>(root[F("full_solar_passthrough_stop_voltage")].as<float>() * 100) / 100.0;



    Configuration.write();

    PowerLimiter.calcNextInverterRestart();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);
}

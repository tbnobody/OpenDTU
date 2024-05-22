// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_powermeter.h"
#include "VeDirectFrameHandler.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "MqttHandleVedirectHass.h"
#include "MqttHandleHass.h"
#include "MqttSettings.h"
#include "PowerLimiter.h"
#include "PowerMeter.h"
#include "PowerMeterHttpJson.h"
#include "PowerMeterHttpSml.h"
#include "WebApi.h"
#include "helper.h"

void WebApiPowerMeterClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/powermeter/status", HTTP_GET, std::bind(&WebApiPowerMeterClass::onStatus, this, _1));
    _server->on("/api/powermeter/config", HTTP_GET, std::bind(&WebApiPowerMeterClass::onAdminGet, this, _1));
    _server->on("/api/powermeter/config", HTTP_POST, std::bind(&WebApiPowerMeterClass::onAdminPost, this, _1));
    _server->on("/api/powermeter/testhttpjsonrequest", HTTP_POST, std::bind(&WebApiPowerMeterClass::onTestHttpJsonRequest, this, _1));
    _server->on("/api/powermeter/testhttpsmlrequest", HTTP_POST, std::bind(&WebApiPowerMeterClass::onTestHttpSmlRequest, this, _1));
}

void WebApiPowerMeterClass::onStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["enabled"] = config.PowerMeter.Enabled;
    root["verbose_logging"] = config.PowerMeter.VerboseLogging;
    root["source"] = config.PowerMeter.Source;

    auto mqtt = root["mqtt"].to<JsonObject>();
    Configuration.serializePowerMeterMqttConfig(config.PowerMeter.Mqtt, mqtt);

    auto serialSdm = root["serial_sdm"].to<JsonObject>();
    Configuration.serializePowerMeterSerialSdmConfig(config.PowerMeter.SerialSdm, serialSdm);

    auto httpJson = root["http_json"].to<JsonObject>();
    Configuration.serializePowerMeterHttpJsonConfig(config.PowerMeter.HttpJson, httpJson);

    auto httpSml = root["http_sml"].to<JsonObject>();
    Configuration.serializePowerMeterHttpSmlConfig(config.PowerMeter.HttpSml, httpSml);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiPowerMeterClass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    this->onStatus(request);
}

void WebApiPowerMeterClass::onAdminPost(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("enabled") && root.containsKey("source"))) {
        retMsg["message"] = "Values are missing!";
        response->setLength();
        request->send(response);
        return;
    }

    auto checkHttpConfig = [&](JsonObject const& cfg) -> bool {
        if (!cfg.containsKey("url")
                || (!cfg["url"].as<String>().startsWith("http://")
                    && !cfg["url"].as<String>().startsWith("https://"))) {
            retMsg["message"] = "URL must either start with http:// or https://!";
            response->setLength();
            request->send(response);
            return false;
        }

        if ((cfg["auth_type"].as<uint8_t>() != HttpRequestConfig::Auth::None)
                && (cfg["username"].as<String>().length() == 0 || cfg["password"].as<String>().length() == 0)) {
            retMsg["message"] = "Username or password must not be empty!";
            response->setLength();
            request->send(response);
            return false;
        }

        if (!cfg.containsKey("timeout")
                || cfg["timeout"].as<uint16_t>() <= 0) {
            retMsg["message"] = "Timeout must be greater than 0 ms!";
            response->setLength();
            request->send(response);
            return false;
        }

        return true;
    };

    if (static_cast<PowerMeterProvider::Type>(root["source"].as<uint8_t>()) == PowerMeterProvider::Type::HTTP_JSON) {
        JsonObject httpJson = root["http_json"];
        JsonArray valueConfigs = httpJson["values"];
        for (uint8_t i = 0; i < valueConfigs.size(); i++) {
            JsonObject valueConfig = valueConfigs[i].as<JsonObject>();

            if (i > 0 && !valueConfig["enabled"].as<bool>()) {
                continue;
            }

            if (i == 0 || httpJson["individual_requests"].as<bool>()) {
                if (!checkHttpConfig(valueConfig["http_request"].as<JsonObject>())) {
                    return;
                }
            }

            if (!valueConfig.containsKey("json_path")
                    || valueConfig["json_path"].as<String>().length() == 0) {
                retMsg["message"] = "Json path must not be empty!";
                response->setLength();
                request->send(response);
                return;
            }
        }
    }

    if (static_cast<PowerMeterProvider::Type>(root["source"].as<uint8_t>()) == PowerMeterProvider::Type::HTTP_SML) {
        JsonObject httpSml = root["http_sml"];
        if (!checkHttpConfig(httpSml["http_request"].as<JsonObject>())) {
            return;
        }
    }

    CONFIG_T& config = Configuration.get();
    config.PowerMeter.Enabled = root["enabled"].as<bool>();
    config.PowerMeter.VerboseLogging = root["verbose_logging"].as<bool>();
    config.PowerMeter.Source = root["source"].as<uint8_t>();

    Configuration.deserializePowerMeterMqttConfig(root["mqtt"].as<JsonObject>(),
            config.PowerMeter.Mqtt);

    Configuration.deserializePowerMeterSerialSdmConfig(root["serial_sdm"].as<JsonObject>(),
            config.PowerMeter.SerialSdm);

    Configuration.deserializePowerMeterHttpJsonConfig(root["http_json"].as<JsonObject>(),
            config.PowerMeter.HttpJson);
    config.PowerMeter.HttpJson.Values[0].Enabled = true;

    Configuration.deserializePowerMeterHttpSmlConfig(root["http_sml"].as<JsonObject>(),
            config.PowerMeter.HttpSml);

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    PowerMeter.updateSettings();
}

void WebApiPowerMeterClass::onTestHttpJsonRequest(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* asyncJsonResponse = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, asyncJsonResponse, root)) {
        return;
    }

    auto& retMsg = asyncJsonResponse->getRoot();

    char response[256];

    auto powerMeterConfig = std::make_unique<CONFIG_T::PowerMeterConfig>();
    JsonObject httpJson = root["http_json"];
    powerMeterConfig->HttpJson.IndividualRequests = httpJson["individual_requests"].as<bool>();
    powerMeterConfig->VerboseLogging = true;
    Configuration.deserializePowerMeterHttpJsonConfig(httpJson,
            powerMeterConfig->HttpJson);
    auto backup = std::make_unique<CONFIG_T::PowerMeterConfig>(Configuration.get().PowerMeter);
    Configuration.get().PowerMeter = *powerMeterConfig;
    auto upMeter = std::make_unique<PowerMeterHttpJson>();
    upMeter->init();
    auto res = upMeter->poll();
    Configuration.get().PowerMeter = *backup;
    using values_t = PowerMeterHttpJson::power_values_t;
    if (std::holds_alternative<values_t>(res)) {
        retMsg["type"] = "success";
        auto vals = std::get<values_t>(res);
        auto pos = snprintf(response, sizeof(response), "Result: %5.2fW", vals[0]);
        for (size_t i = 1; i < POWERMETER_HTTP_JSON_MAX_VALUES; ++i) {
            if (!powerMeterConfig->HttpJson.Values[i].Enabled) { continue; }
            pos += snprintf(response + pos, sizeof(response) - pos, ", %5.2fW", vals[i]);
        }
        snprintf(response + pos, sizeof(response) - pos, ", Total: %5.2f", upMeter->getPowerTotal());
    } else {
        snprintf(response, sizeof(response), "%s", std::get<String>(res).c_str());
    }

    retMsg["message"] = response;
    asyncJsonResponse->setLength();
    request->send(asyncJsonResponse);
}

void WebApiPowerMeterClass::onTestHttpSmlRequest(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* asyncJsonResponse = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, asyncJsonResponse, root)) {
        return;
    }

    auto& retMsg = asyncJsonResponse->getRoot();

    char response[256];

    auto powerMeterConfig = std::make_unique<CONFIG_T::PowerMeterConfig>();
    Configuration.deserializePowerMeterHttpSmlConfig(root["http_sml"].as<JsonObject>(),
            powerMeterConfig->HttpSml);
    powerMeterConfig->VerboseLogging = true;
    auto backup = std::make_unique<CONFIG_T::PowerMeterConfig>(Configuration.get().PowerMeter);
    Configuration.get().PowerMeter = *powerMeterConfig;
    auto upMeter = std::make_unique<PowerMeterHttpSml>();
    upMeter->init();
    auto res = upMeter->poll();
    Configuration.get().PowerMeter = *backup;
    if (res.isEmpty()) {
        retMsg["type"] = "success";
        snprintf(response, sizeof(response), "Result: %5.2fW", upMeter->getPowerTotal());
    } else {
        snprintf(response, sizeof(response), "%s", res.c_str());
    }

    retMsg["message"] = response;
    asyncJsonResponse->setLength();
    request->send(asyncJsonResponse);
}

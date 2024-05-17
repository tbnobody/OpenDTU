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
    root["interval"] = config.PowerMeter.Interval;
    root["mqtt_topic_powermeter_1"] = config.PowerMeter.MqttTopicPowerMeter1;
    root["mqtt_topic_powermeter_2"] = config.PowerMeter.MqttTopicPowerMeter2;
    root["mqtt_topic_powermeter_3"] = config.PowerMeter.MqttTopicPowerMeter3;
    root["sdmaddress"] = config.PowerMeter.SdmAddress;
    root["http_individual_requests"] = config.PowerMeter.HttpIndividualRequests;

    auto httpSml = root["http_sml"].to<JsonObject>();
    Configuration.serializePowerMeterHttpSmlConfig(config.PowerMeter.HttpSml, httpSml);

    auto httpJson = root["http_json"].to<JsonArray>();
    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto valueConfig = httpJson.add<JsonObject>();
        valueConfig["index"] = i + 1;
        Configuration.serializePowerMeterHttpJsonConfig(config.PowerMeter.HttpJson[i], valueConfig);
    }

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
        JsonArray httpJson = root["http_json"];
        for (uint8_t i = 0; i < httpJson.size(); i++) {
            JsonObject valueConfig = httpJson[i].as<JsonObject>();

            if (i > 0 && !valueConfig["enabled"].as<bool>()) {
                continue;
            }

            if (i == 0 || valueConfig["http_individual_requests"].as<bool>()) {
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
    config.PowerMeter.Interval = root["interval"].as<uint32_t>();
    strlcpy(config.PowerMeter.MqttTopicPowerMeter1, root["mqtt_topic_powermeter_1"].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter1));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter2, root["mqtt_topic_powermeter_2"].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter2));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter3, root["mqtt_topic_powermeter_3"].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter3));
    config.PowerMeter.SdmAddress = root["sdmaddress"].as<uint8_t>();
    config.PowerMeter.HttpIndividualRequests = root["http_individual_requests"].as<bool>();

    Configuration.deserializePowerMeterHttpSmlConfig(root["http_sml"].as<JsonObject>(),
            config.PowerMeter.HttpSml);

    JsonArray httpJson = root["http_json"];
    for (uint8_t i = 0; i < httpJson.size(); i++) {
        Configuration.deserializePowerMeterHttpJsonConfig(httpJson[i].as<JsonObject>(),
                config.PowerMeter.HttpJson[i]);
    }
    config.PowerMeter.HttpJson[0].Enabled = true;

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
    powerMeterConfig->HttpIndividualRequests = root["http_individual_requests"].as<bool>();
    JsonArray httpJson = root["http_json"];
    for (uint8_t i = 0; i < httpJson.size(); i++) {
        Configuration.deserializePowerMeterHttpJsonConfig(httpJson[i].as<JsonObject>(),
                powerMeterConfig->HttpJson[i]);
    }
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
            if (!powerMeterConfig->HttpJson[i].Enabled) { continue; }
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

    if (!root.containsKey("url") || !root.containsKey("username") || !root.containsKey("password") 
            || !root.containsKey("timeout")) {
        retMsg["message"] = "Missing fields!";
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }


    char response[256];

    PowerMeterHttpSmlConfig httpSmlConfig;
    Configuration.deserializePowerMeterHttpSmlConfig(root.as<JsonObject>(), httpSmlConfig);
    auto upMeter = std::make_unique<PowerMeterHttpSml>();
    if (upMeter->query(httpSmlConfig.HttpRequest)) {
        retMsg["type"] = "success";
        snprintf_P(response, sizeof(response), "Success! Power: %5.2fW", upMeter->getPowerTotal());
    } else {
        snprintf_P(response, sizeof(response), "%s", upMeter->tibberPowerMeterError);
    }

    retMsg["message"] = response;
    asyncJsonResponse->setLength();
    request->send(asyncJsonResponse);
}

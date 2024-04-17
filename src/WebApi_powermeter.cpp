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
#include "HttpPowerMeter.h"
#include "WebApi.h"
#include "helper.h"

void WebApiPowerMeterClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/powermeter/status", HTTP_GET, std::bind(&WebApiPowerMeterClass::onStatus, this, _1));
    _server->on("/api/powermeter/config", HTTP_GET, std::bind(&WebApiPowerMeterClass::onAdminGet, this, _1));
    _server->on("/api/powermeter/config", HTTP_POST, std::bind(&WebApiPowerMeterClass::onAdminPost, this, _1));
    _server->on("/api/powermeter/testhttprequest", HTTP_POST, std::bind(&WebApiPowerMeterClass::onTestHttpRequest, this, _1));
}

void WebApiPowerMeterClass::decodeJsonPhaseConfig(JsonObject const& json, PowerMeterHttpConfig& config) const
{
    config.Enabled = json["enabled"].as<bool>();
    strlcpy(config.Url, json["url"].as<String>().c_str(), sizeof(config.Url));
    config.AuthType = json["auth_type"].as<PowerMeterHttpConfig::Auth>();
    strlcpy(config.Username, json["username"].as<String>().c_str(), sizeof(config.Username));
    strlcpy(config.Password, json["password"].as<String>().c_str(), sizeof(config.Password));
    strlcpy(config.HeaderKey, json["header_key"].as<String>().c_str(), sizeof(config.HeaderKey));
    strlcpy(config.HeaderValue, json["header_value"].as<String>().c_str(), sizeof(config.HeaderValue));
    config.Timeout = json["timeout"].as<uint16_t>();
    strlcpy(config.JsonPath, json["json_path"].as<String>().c_str(), sizeof(config.JsonPath));
    config.PowerUnit = json["unit"].as<PowerMeterHttpConfig::Unit>();
    config.SignInverted = json["sign_inverted"].as<bool>();
}

void WebApiPowerMeterClass::onStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 2048);
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["enabled"] = config.PowerMeter.Enabled;
    root["verbose_logging"] = config.PowerMeter.VerboseLogging;
    root["source"] = config.PowerMeter.Source;
    root["interval"] = config.PowerMeter.Interval;
    root["mqtt_topic_powermeter_1"] = config.PowerMeter.MqttTopicPowerMeter1;
    root["mqtt_topic_powermeter_2"] = config.PowerMeter.MqttTopicPowerMeter2;
    root["mqtt_topic_powermeter_3"] = config.PowerMeter.MqttTopicPowerMeter3;
    root["sdmbaudrate"] = config.PowerMeter.SdmBaudrate;
    root["sdmaddress"] = config.PowerMeter.SdmAddress;
    root["http_individual_requests"] = config.PowerMeter.HttpIndividualRequests;

    JsonArray httpPhases = root.createNestedArray("http_phases");

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject phaseObject = httpPhases.createNestedObject();

        phaseObject["index"] = i + 1;
        phaseObject["enabled"] = config.PowerMeter.Http_Phase[i].Enabled;
        phaseObject["url"] = String(config.PowerMeter.Http_Phase[i].Url);
        phaseObject["auth_type"]= config.PowerMeter.Http_Phase[i].AuthType;
        phaseObject["username"] = String(config.PowerMeter.Http_Phase[i].Username);
        phaseObject["password"] = String(config.PowerMeter.Http_Phase[i].Password);
        phaseObject["header_key"] = String(config.PowerMeter.Http_Phase[i].HeaderKey);
        phaseObject["header_value"] = String(config.PowerMeter.Http_Phase[i].HeaderValue);
        phaseObject["timeout"] = config.PowerMeter.Http_Phase[i].Timeout;
        phaseObject["json_path"] = String(config.PowerMeter.Http_Phase[i].JsonPath);
        phaseObject["unit"] = config.PowerMeter.Http_Phase[i].PowerUnit;
        phaseObject["sign_inverted"] = config.PowerMeter.Http_Phase[i].SignInverted;
    }

    response->setLength();
    request->send(response);
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
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 4096) {
        retMsg["message"] = "Data too large!";
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(4096);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("enabled") && root.containsKey("source"))) {
        retMsg["message"] = "Values are missing!";
        response->setLength();
        request->send(response);
        return;
    }

    if (static_cast<PowerMeterClass::Source>(root["source"].as<uint8_t>()) == PowerMeterClass::Source::HTTP) {
        JsonArray http_phases = root["http_phases"];
        for (uint8_t i = 0; i < http_phases.size(); i++) {
            JsonObject phase = http_phases[i].as<JsonObject>();

            if (i > 0 && !phase["enabled"].as<bool>()) {
                continue;
            }

            if (i == 0 || phase["http_individual_requests"].as<bool>()) {
                if (!phase.containsKey("url")
                        || (!phase["url"].as<String>().startsWith("http://")
                        && !phase["url"].as<String>().startsWith("https://"))) {
                    retMsg["message"] = "URL must either start with http:// or https://!";
                    response->setLength();
                    request->send(response);
                    return;
                }

                if ((phase["auth_type"].as<uint8_t>() != PowerMeterHttpConfig::Auth::None)
                    && ( phase["username"].as<String>().length() == 0 ||  phase["password"].as<String>().length() == 0)) {
                    retMsg["message"] = "Username or password must not be empty!";
                    response->setLength();
                    request->send(response);
                    return;
                }

                if (!phase.containsKey("timeout")
                        || phase["timeout"].as<uint16_t>() <= 0) {
                    retMsg["message"] = "Timeout must be greater than 0 ms!";
                    response->setLength();
                    request->send(response);
                    return;
                }
            }

            if (!phase.containsKey("json_path")
                    || phase["json_path"].as<String>().length() == 0) {
                retMsg["message"] = "Json path must not be empty!";
                response->setLength();
                request->send(response);
                return;
            }
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
    config.PowerMeter.SdmBaudrate = root["sdmbaudrate"].as<uint32_t>();
    config.PowerMeter.SdmAddress = root["sdmaddress"].as<uint8_t>();
    config.PowerMeter.HttpIndividualRequests = root["http_individual_requests"].as<bool>();

    JsonArray http_phases = root["http_phases"];
    for (uint8_t i = 0; i < http_phases.size(); i++) {
        decodeJsonPhaseConfig(http_phases[i].as<JsonObject>(), config.PowerMeter.Http_Phase[i]);
    }
    config.PowerMeter.Http_Phase[0].Enabled = true;

    WebApi.writeConfig(retMsg);

    response->setLength();
    request->send(response);

    // reboot requiered as per https://github.com/helgeerbe/OpenDTU-OnBattery/issues/565#issuecomment-1872552559
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

void WebApiPowerMeterClass::onTestHttpRequest(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* asyncJsonResponse = new AsyncJsonResponse();
    auto& retMsg = asyncJsonResponse->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 2048) {
        retMsg["message"] = "Data too large!";
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    DynamicJsonDocument root(2048);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    if (!root.containsKey("url") || !root.containsKey("auth_type") || !root.containsKey("username") || !root.containsKey("password") 
            || !root.containsKey("header_key") || !root.containsKey("header_value")
            || !root.containsKey("timeout") || !root.containsKey("json_path")) {
        retMsg["message"] = "Missing fields!";
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }


    char response[256];

    int phase = 0;//"absuing" index 0 of the float power[3] in HttpPowerMeter to store the result
    PowerMeterHttpConfig phaseConfig;
    decodeJsonPhaseConfig(root.as<JsonObject>(), phaseConfig);
    if (HttpPowerMeter.queryPhase(phase, phaseConfig)) {
        retMsg["type"] = "success";
        snprintf_P(response, sizeof(response), "Success! Power: %5.2fW", HttpPowerMeter.getPower(phase + 1));
    } else {
        snprintf_P(response, sizeof(response), "%s", HttpPowerMeter.httpPowerMeterError);
    }

    retMsg["message"] = response;
    asyncJsonResponse->setLength();
    request->send(asyncJsonResponse);
}

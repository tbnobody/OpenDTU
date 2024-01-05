// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
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

void WebApiPowerMeterClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/powermeter/status", HTTP_GET, std::bind(&WebApiPowerMeterClass::onStatus, this, _1));
    _server->on("/api/powermeter/config", HTTP_GET, std::bind(&WebApiPowerMeterClass::onAdminGet, this, _1));
    _server->on("/api/powermeter/config", HTTP_POST, std::bind(&WebApiPowerMeterClass::onAdminPost, this, _1));
    _server->on("/api/powermeter/testhttprequest", HTTP_POST, std::bind(&WebApiPowerMeterClass::onTestHttpRequest, this, _1));
}

void WebApiPowerMeterClass::loop()
{
}

void WebApiPowerMeterClass::onStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 2048);
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("enabled")] = config.PowerMeter.Enabled;
    root[F("verbose_logging")] = config.PowerMeter.VerboseLogging;
    root[F("source")] = config.PowerMeter.Source;
    root[F("interval")] = config.PowerMeter.Interval;
    root[F("mqtt_topic_powermeter_1")] = config.PowerMeter.MqttTopicPowerMeter1;
    root[F("mqtt_topic_powermeter_2")] = config.PowerMeter.MqttTopicPowerMeter2;
    root[F("mqtt_topic_powermeter_3")] = config.PowerMeter.MqttTopicPowerMeter3;
    root[F("sdmbaudrate")] = config.PowerMeter.SdmBaudrate;
    root[F("sdmaddress")] = config.PowerMeter.SdmAddress;
    root[F("http_individual_requests")] = config.PowerMeter.HttpIndividualRequests;

    JsonArray httpPhases = root.createNestedArray(F("http_phases"));

    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject phaseObject = httpPhases.createNestedObject();

        phaseObject[F("index")] = i + 1;
        phaseObject[F("enabled")] = config.PowerMeter.Http_Phase[i].Enabled;
        phaseObject[F("url")] = String(config.PowerMeter.Http_Phase[i].Url);
        phaseObject[F("auth_type")]= config.PowerMeter.Http_Phase[i].AuthType;
        phaseObject[F("username")] = String(config.PowerMeter.Http_Phase[i].Username);
        phaseObject[F("password")] = String(config.PowerMeter.Http_Phase[i].Password);
        phaseObject[F("header_key")] = String(config.PowerMeter.Http_Phase[i].HeaderKey);
        phaseObject[F("header_value")] = String(config.PowerMeter.Http_Phase[i].HeaderValue);
        phaseObject[F("json_path")] = String(config.PowerMeter.Http_Phase[i].JsonPath);
        phaseObject[F("timeout")] = config.PowerMeter.Http_Phase[i].Timeout;
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
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 4096) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(4096);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("enabled") && root.containsKey("source"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("source")].as<uint8_t>() == PowerMeter.SOURCE_HTTP) {
        JsonArray http_phases = root[F("http_phases")];
        for (uint8_t i = 0; i < http_phases.size(); i++) {
            JsonObject phase = http_phases[i].as<JsonObject>();

            if (i > 0 && !phase[F("enabled")].as<bool>()) {
                continue;
            }

            if (i == 0 || phase[F("http_individual_requests")].as<bool>()) {
                if (!phase.containsKey("url")
                        || (!phase[F("url")].as<String>().startsWith("http://")
                        && !phase[F("url")].as<String>().startsWith("https://"))) {
                    retMsg[F("message")] = F("URL must either start with http:// or https://!");
                    response->setLength();
                    request->send(response);
                    return;
                }

                if ((phase[F("auth_type")].as<Auth>() != Auth::none)
                    && ( phase[F("username")].as<String>().length() == 0 ||  phase[F("password")].as<String>().length() == 0)) {
                    retMsg[F("message")] = F("Username or password must not be empty!");
                    response->setLength();
                    request->send(response);
                    return;
                }

                if (!phase.containsKey("timeout")
                        || phase[F("timeout")].as<uint16_t>() <= 0) {
                    retMsg[F("message")] = F("Timeout must be greater than 0 ms!");
                    response->setLength();
                    request->send(response);
                    return;
                }
            }

            if (!phase.containsKey("json_path")
                    || phase[F("json_path")].as<String>().length() == 0) {
                retMsg[F("message")] = F("Json path must not be empty!");
                response->setLength();
                request->send(response);
                return;
            }
        }
    }

    CONFIG_T& config = Configuration.get();
    config.PowerMeter.Enabled = root[F("enabled")].as<bool>();
    config.PowerMeter.VerboseLogging = root[F("verbose_logging")].as<bool>();
    config.PowerMeter.Source = root[F("source")].as<uint8_t>();
    config.PowerMeter.Interval = root[F("interval")].as<uint32_t>();
    strlcpy(config.PowerMeter.MqttTopicPowerMeter1, root[F("mqtt_topic_powermeter_1")].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter1));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter2, root[F("mqtt_topic_powermeter_2")].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter2));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter3, root[F("mqtt_topic_powermeter_3")].as<String>().c_str(), sizeof(config.PowerMeter.MqttTopicPowerMeter3));
    config.PowerMeter.SdmBaudrate = root[F("sdmbaudrate")].as<uint32_t>();
    config.PowerMeter.SdmAddress = root[F("sdmaddress")].as<uint8_t>();
    config.PowerMeter.HttpIndividualRequests = root[F("http_individual_requests")].as<bool>();

    JsonArray http_phases = root[F("http_phases")];
    for (uint8_t i = 0; i < http_phases.size(); i++) {
        JsonObject phase = http_phases[i].as<JsonObject>();

        config.PowerMeter.Http_Phase[i].Enabled = (i == 0 ? true : phase[F("enabled")].as<bool>());
        strlcpy(config.PowerMeter.Http_Phase[i].Url, phase[F("url")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].Url));
        config.PowerMeter.Http_Phase[i].AuthType = phase[F("auth_type")].as<Auth>();
        strlcpy(config.PowerMeter.Http_Phase[i].Username, phase[F("username")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].Username));
        strlcpy(config.PowerMeter.Http_Phase[i].Password, phase[F("password")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].Password));
        strlcpy(config.PowerMeter.Http_Phase[i].HeaderKey, phase[F("header_key")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].HeaderKey));
        strlcpy(config.PowerMeter.Http_Phase[i].HeaderValue, phase[F("header_value")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].HeaderValue));
        config.PowerMeter.Http_Phase[i].Timeout = phase[F("timeout")].as<uint16_t>();
        strlcpy(config.PowerMeter.Http_Phase[i].JsonPath, phase[F("json_path")].as<String>().c_str(), sizeof(config.PowerMeter.Http_Phase[i].JsonPath));
    }

    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

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
    JsonObject retMsg = asyncJsonResponse->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 2048) {
        retMsg[F("message")] = F("Data too large!");
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    DynamicJsonDocument root(2048);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }

    if (!root.containsKey("url") || !root.containsKey("auth_type") || !root.containsKey("username") || !root.containsKey("password") 
            || !root.containsKey("header_key") || !root.containsKey("header_value")
            || !root.containsKey("timeout") || !root.containsKey("json_path")) {
        retMsg[F("message")] = F("Missing fields!");
        asyncJsonResponse->setLength();
        request->send(asyncJsonResponse);
        return;
    }


    char response[256];

    String urlProtocol;
    String urlHostname;
    String urlUri;

    HttpPowerMeter.extractUrlComponents(root[F("url")].as<String>().c_str(), urlProtocol, urlHostname, urlUri);

    int phase = 0;//"absuing" index 0 of the float power[3] in HttpPowerMeter to store the result
    if (HttpPowerMeter.queryPhase(phase, urlProtocol, urlHostname, urlUri,
            root[F("auth_type")].as<Auth>(), root[F("username")].as<String>().c_str(), root[F("password")].as<String>().c_str(),
            root[F("header_key")].as<String>().c_str(), root[F("header_value")].as<String>().c_str(), root[F("timeout")].as<uint16_t>(),
            root[F("json_path")].as<String>().c_str())) {
        retMsg[F("type")] = F("success");
        snprintf_P(response, sizeof(response), "Success! Power: %5.2fW", HttpPowerMeter.getPower(phase + 1));
    } else {
        snprintf_P(response, sizeof(response), "%s", HttpPowerMeter.httpPowerMeterError);
    }

    retMsg[F("message")] = F(response);
    asyncJsonResponse->setLength();
    request->send(asyncJsonResponse);
}

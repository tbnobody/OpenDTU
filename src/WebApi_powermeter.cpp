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
#include "WebApi.h"
#include "helper.h"

void WebApiPowerMeterClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/powermeter/status", HTTP_GET, std::bind(&WebApiPowerMeterClass::onStatus, this, _1));
    _server->on("/api/powermeter/config", HTTP_GET, std::bind(&WebApiPowerMeterClass::onAdminGet, this, _1));
    _server->on("/api/powermeter/config", HTTP_POST, std::bind(&WebApiPowerMeterClass::onAdminPost, this, _1));
}

void WebApiPowerMeterClass::loop()
{
}

void WebApiPowerMeterClass::onStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("enabled")] = config.PowerMeter_Enabled;
    root[F("source")] = config.PowerMeter_Source;
    root[F("interval")] = config.PowerMeter_Interval;
    root[F("mqtt_topic_powermeter_1")] = config.PowerMeter_MqttTopicPowerMeter1;
    root[F("mqtt_topic_powermeter_2")] = config.PowerMeter_MqttTopicPowerMeter2;
    root[F("mqtt_topic_powermeter_3")] = config.PowerMeter_MqttTopicPowerMeter3;
    root[F("sdmbaudrate")] = config.PowerMeter_SdmBaudrate;
    root[F("sdmaddress")] = config.PowerMeter_SdmAddress;

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

    if (!(root.containsKey("enabled") && root.containsKey("source"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.PowerMeter_Enabled = root[F("enabled")].as<bool>();
    config.PowerMeter_Source = root[F("source")].as<uint8_t>();
    config.PowerMeter_Interval = root[F("interval")].as<uint32_t>();
    strlcpy(config.PowerMeter_MqttTopicPowerMeter1, root[F("mqtt_topic_powermeter_1")].as<String>().c_str(), sizeof(config.PowerMeter_MqttTopicPowerMeter1));
    strlcpy(config.PowerMeter_MqttTopicPowerMeter2, root[F("mqtt_topic_powermeter_2")].as<String>().c_str(), sizeof(config.PowerMeter_MqttTopicPowerMeter2));
    strlcpy(config.PowerMeter_MqttTopicPowerMeter3, root[F("mqtt_topic_powermeter_3")].as<String>().c_str(), sizeof(config.PowerMeter_MqttTopicPowerMeter3));
    config.PowerMeter_SdmBaudrate = root[F("sdmbaudrate")].as<uint32_t>();
    config.PowerMeter_SdmAddress = root[F("sdmaddress")].as<uint8_t>();
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);


    MqttSettings.performReconnect();  
    PowerMeter.init();
    PowerLimiter.init();
    MqttHandleHass.forceUpdate();
    MqttHandleVedirectHass.forceUpdate();
}

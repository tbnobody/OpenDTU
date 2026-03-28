// SPDX-License-Identifier: GPL-2.0-or-later
#include "WebApi_solarcharger.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include "MqttHandlePowerLimiterHass.h"
#include <solarcharger/Controller.h>

void WebApiSolarChargerlass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/solarcharger/config", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiSolarChargerlass::onAdminGet, this, _1)));
    _server->on("/api/solarcharger/config", HTTP_POST, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiSolarChargerlass::onAdminPost, this, _1)));
}

void WebApiSolarChargerlass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto const& config = Configuration.get();

    ConfigurationClass::serializeSolarChargerConfig(config.SolarCharger, root);

    auto mqtt = root["mqtt"].to<JsonObject>();
    ConfigurationClass::serializeSolarChargerMqttConfig(config.SolarCharger.Mqtt, mqtt);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiSolarChargerlass::onAdminPost(AsyncWebServerRequest* request)
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

    if (!root["enabled"].is<bool>() ||
            !root["provider"].is<uint8_t>() ||
            !root["publish_updates_only"].is<bool>()) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();
        ConfigurationClass::deserializeSolarChargerConfig(root.as<JsonObject>(), config.SolarCharger);

        ConfigurationClass::deserializeSolarChargerMqttConfig(root["mqtt"].as<JsonObject>(), config.SolarCharger.Mqtt);
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    SolarCharger.updateSettings();

    // potentially make solar passthrough thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

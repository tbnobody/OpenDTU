// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_gridcharger.h"
#include <gridcharger/Controller.h>
#include <gridcharger/huawei/Provider.h>
#include "Configuration.h"
#include "PinMapping.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiGridChargerClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/gridcharger/status", HTTP_GET, std::bind(&WebApiGridChargerClass::onStatus, this, _1));
    _server->on("/api/gridcharger/config", HTTP_GET, std::bind(&WebApiGridChargerClass::onAdminGet, this, _1));
    _server->on("/api/gridcharger/config", HTTP_POST, std::bind(&WebApiGridChargerClass::onAdminPost, this, _1));
    _server->on("/api/gridcharger/limit", HTTP_POST, std::bind(&WebApiGridChargerClass::onLimitPost, this, _1));
    _server->on("/api/gridcharger/power", HTTP_POST, std::bind(&WebApiGridChargerClass::onPowerPost, this, _1));
}

void WebApiGridChargerClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    GridCharger.getStats()->getLiveViewData(root);

    response->setLength();
    request->send(response);
}

void WebApiGridChargerClass::onLimitPost(AsyncWebServerRequest* request)
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

    using Setting = GridChargers::Huawei::HardwareInterface::Setting;

    auto applySetting = [&](const char* key, float min, float max, WebApiError error, Setting setting) -> bool {
        if (!root[key].is<float>()) { return true; }

        auto value = root[key].as<float>();
        if (value < min || value > max) {
            retMsg["message"] = String(key) + " out of range [" +
                String(min) + ", " + String(max) + "]";
            retMsg["code"] = error;
            retMsg["param"]["min"] = min;
            retMsg["param"]["max"] = max;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return false;
        }

        // Only call Huawei-specific methods when Huawei provider is active
        auto const& config = Configuration.get();
        if (config.GridCharger.Provider == GridChargerProviderType::HUAWEI) {
            auto* huaweiProvider = GridCharger.getProvider<GridChargers::Huawei::Provider>();
            if (huaweiProvider) {
                huaweiProvider->setParameter(value, setting);
                return true;
            }
        }

        retMsg["message"] = "Current provider does not support this feature!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return false;
    };

    using Provider = GridChargers::Huawei::Provider;

    if (!applySetting("voltage",
        Provider::MIN_ONLINE_VOLTAGE,
        Provider::MAX_ONLINE_VOLTAGE,
        WebApiError::R48xxVoltageLimitOutOfRange,
        Setting::OnlineVoltage)) {
        return;
    }

    if (!applySetting("current",
        Provider::MIN_ONLINE_CURRENT,
        Provider::MAX_ONLINE_CURRENT,
        WebApiError::R48xxCurrentLimitOutOfRange,
        Setting::OnlineCurrent)) {
        return;
    }

    retMsg["type"] = "success";
    retMsg["message"] = "Limits applied!";
    retMsg["code"] = WebApiError::GenericSuccess;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiGridChargerClass::onPowerPost(AsyncWebServerRequest* request)
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

    if (!root["power"].is<bool>()) {
        retMsg["message"] = "Value missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    bool power = root["power"].as<bool>();

    // Only call Huawei-specific methods when Huawei provider is active
    auto const& config = Configuration.get();
    if (config.GridCharger.Provider == GridChargerProviderType::HUAWEI) {
        auto* huaweiProvider = GridCharger.getProvider<GridChargers::Huawei::Provider>();
        if (huaweiProvider) {
            huaweiProvider->setProduction(power);

            retMsg["type"] = "success";
            retMsg["message"] = "Power production " + String(power ? "en" : "dis") + "abled!";
            retMsg["code"] = WebApiError::GenericSuccess;

            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }
    }

    retMsg["message"] = "Current provider does not support this feature!";
    retMsg["code"] = WebApiError::GenericNoValueFound;
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiGridChargerClass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto const& config = Configuration.get();

    ConfigurationClass::serializeGridChargerConfig(config.GridCharger, root);

    auto can = root["can"].to<JsonObject>();
    ConfigurationClass::serializeGridChargerCanConfig(config.GridCharger.Can, can);

    auto huawei = root["huawei"].to<JsonObject>();
    ConfigurationClass::serializeGridChargerHuaweiConfig(config.GridCharger.Huawei, huawei);

    auto trucki = root["trucki"].to<JsonObject>();
    ConfigurationClass::serializeGridChargerTruckiConfig(config.GridCharger.Trucki, trucki);

    response->setLength();
    request->send(response);
}

void WebApiGridChargerClass::onAdminPost(AsyncWebServerRequest* request)
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

    if (!(root["enabled"].is<bool>()) ||
        !(root["provider"].is<uint8_t>()) ||
        !(root["can"]["controller_frequency"].is<uint32_t>()) ||
        !(root["auto_power_enabled"].is<bool>()) ||
        !(root["emergency_charge_enabled"].is<bool>()) ||
        !(root["huawei"]["offline_voltage"].is<float>()) ||
        !(root["huawei"]["offline_current"].is<float>()) ||
        !(root["huawei"]["input_current_limit"].is<float>()) ||
        !(root["huawei"]["fan_online_full_speed"].is<bool>()) ||
        !(root["huawei"]["fan_offline_full_speed"].is<bool>()) ||
        !(root["trucki"]["ip_address"].is<const char*>()) ||
        !(root["trucki"]["password"].is<const char*>()) ||
        !(root["voltage_limit"].is<float>()) ||
        !(root["lower_power_limit"].is<float>()) ||
        !(root["upper_power_limit"].is<float>())) {
        retMsg["message"] = "Values are missing or of wrong type!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    using HuaweiProvider = GridChargers::Huawei::Provider;

    auto isValidRange = [&](const char* valueName, float min, float max, WebApiError error) -> bool {
        if (root["huawei"][valueName].as<float>() < min || root["huawei"][valueName].as<float>() > max) {
            retMsg["message"] = String(valueName) + " out of range [" + String(min) + ", " + String(max) + "]";
            retMsg["code"] = error;
            retMsg["param"]["min"] = min;
            retMsg["param"]["max"] = max;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return false;
        }
        return true;
    };

    if (!isValidRange("offline_voltage", HuaweiProvider::MIN_OFFLINE_VOLTAGE, HuaweiProvider::MAX_OFFLINE_VOLTAGE, WebApiError::R48xxVoltageLimitOutOfRange) ||
        !isValidRange("offline_current", HuaweiProvider::MIN_OFFLINE_CURRENT, HuaweiProvider::MAX_OFFLINE_CURRENT, WebApiError::R48xxCurrentLimitOutOfRange) ||
        !isValidRange("input_current_limit", HuaweiProvider::MIN_INPUT_CURRENT_LIMIT, HuaweiProvider::MAX_INPUT_CURRENT_LIMIT, WebApiError::R48xxCurrentLimitOutOfRange)) {
        return;
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();
        ConfigurationClass::deserializeGridChargerConfig(root.as<JsonObject>(), config.GridCharger);
        ConfigurationClass::deserializeGridChargerCanConfig(root["can"].as<JsonObject>(), config.GridCharger.Can);
        ConfigurationClass::deserializeGridChargerHuaweiConfig(root["huawei"].as<JsonObject>(), config.GridCharger.Huawei);
        ConfigurationClass::deserializeGridChargerTruckiConfig(root["trucki"].as<JsonObject>(), config.GridCharger.Trucki);
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    GridCharger.updateSettings();
}

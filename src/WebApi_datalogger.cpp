// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_datalogger.h"
#include "Configuration.h"
#include "Utils.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiDataLoggerClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/datalogger/config", HTTP_GET, std::bind(&WebApiDataLoggerClass::onDataLoggerAdminGet, this, _1));
    server.on("/api/datalogger/config", HTTP_POST, std::bind(&WebApiDataLoggerClass::onDataLoggerAdminPost, this, _1));
}

void WebApiDataLoggerClass::onDataLoggerAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["datalogger_enabled"] = config.DataLogger.Enabled;
    root["saveinterval"] = config.DataLogger.SaveInterval;
    root["filename"] = config.DataLogger.FileName;

    auto outputConfig = root["output_config"].to<JsonObject>();
    outputConfig["total_yield_total"] = config.DataLogger.OutputConfig.TotalYieldTotal;
    outputConfig["total_yield_day"] = config.DataLogger.OutputConfig.TotalYieldDay;
    outputConfig["total_power"] = config.DataLogger.OutputConfig.TotalPower;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiDataLoggerClass::onDataLoggerAdminPost(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("datalogger_enabled")
            && root.containsKey("saveinterval")
            && root.containsKey("filename")
            && root.containsKey("output_config"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    CONFIG_T& config = Configuration.get();

    config.DataLogger.Enabled = root["datalogger_enabled"].as<bool>();
    config.DataLogger.SaveInterval = root["saveinterval"].as<uint32_t>();
    strlcpy(config.DataLogger.FileName, root["filename"].as<String>().c_str(), sizeof(config.DataLogger.FileName));

    config.DataLogger.OutputConfig.TotalYieldTotal = root["output_config"]["total_yield_total"].as<bool>();
    config.DataLogger.OutputConfig.TotalYieldDay = root["output_config"]["total_yield_day"].as<bool>();
    config.DataLogger.OutputConfig.TotalPower = root["output_config"]["total_power"].as<bool>();

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    Utils::restartDtu();
}

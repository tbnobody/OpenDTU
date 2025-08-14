// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Thomas Basler and others
 */
#include "WebApi_logging.h"
#include "Configuration.h"
#include "Logging.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

#include <vector>

void WebApiLoggingClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/logging/config", HTTP_GET, std::bind(&WebApiLoggingClass::onLoggingAdminGet, this, _1));
    server.on("/api/logging/config", HTTP_POST, std::bind(&WebApiLoggingClass::onLoggingAdminPost, this, _1));
}

void WebApiLoggingClass::onLoggingAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    auto& configurableModules = Logging.getConfigurableModules();

    JsonObject loglevel = root["loglevel"].to<JsonObject>();
    loglevel["default"] = config.Logging.Default;

    JsonArray logModules = loglevel["modules"].to<JsonArray>();
    for (const auto& availModule : configurableModules) {
        JsonObject logModule = logModules.add<JsonObject>();
        logModule["name"] = availModule;

        int8_t idx = Configuration.getIndexForLogModule(availModule);
        // Set to inherit if unknown
        logModule["level"] = idx < 0 || idx > ESP_LOG_VERBOSE ? -1 : config.Logging.Modules[idx].Level;
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiLoggingClass::onLoggingAdminPost(AsyncWebServerRequest* request)
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
    auto& configurableModules = Logging.getConfigurableModules();

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        config.Logging.Default = std::max<int8_t>(ESP_LOG_NONE, std::min<int8_t>(ESP_LOG_VERBOSE, root["loglevel"]["default"].as<int8_t>()));

        for (uint8_t i = 0; i < LOG_MODULE_COUNT; i++) {
            config.Logging.Modules[i].Level = ESP_LOG_NONE;
            config.Logging.Modules[i].Name[0] = '\0';
        }

        JsonArray logmodules = root["loglevel"]["modules"].as<JsonArray>();
        uint8_t i = 0;
        for (const auto& logmodule : logmodules) {
            bool isValidModule = std::find(configurableModules.begin(), configurableModules.end(), logmodule["name"] | "") != configurableModules.end();
            if (!isValidModule) {
                continue;
            }

            int8_t level = std::max<int8_t>(-1, std::min<int8_t>(ESP_LOG_VERBOSE, logmodule["level"].as<int8_t>()));
            if (level < ESP_LOG_NONE) {
                // Skip modules set to inherit
                continue;
            }

            config.Logging.Modules[i].Level = level;
            strlcpy(config.Logging.Modules[i].Name, logmodule["name"] | "", sizeof(config.Logging.Modules[i].Name));

            if (++i >= LOG_MODULE_COUNT) {
                break;
            }
        }
    }

    Logging.applyLogLevels();

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_integrations.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiIntegrationsClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/integrations/config", HTTP_GET, std::bind(&WebApiIntegrationsClass::onIntegrationsAdminGet, this, _1));
    server.on("/api/integrations/config", HTTP_POST, std::bind(&WebApiIntegrationsClass::onIntegrationsAdminPost, this, _1));
}

void WebApiIntegrationsClass::onIntegrationsAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["goe_ctrl_hostname"] = config.Integrations.GoeControllerHostname;
    root["goe_ctrl_enabled"] = config.Integrations.GoeControllerEnabled;
    root["goe_ctrl_publish_home_category"] = config.Integrations.GoeControllerPublishHomeCategory;
    root["goe_ctrl_update_interval"] = config.Integrations.GoeControllerUpdateInterval;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiIntegrationsClass::onIntegrationsAdminPost(AsyncWebServerRequest* request)
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

    if (!(root["goe_ctrl_hostname"].is<String>()
            && root["goe_ctrl_enabled"].is<bool>()
            && root["goe_ctrl_publish_home_category"].is<bool>()
            && root["goe_ctrl_update_interval"].is<uint32_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["goe_ctrl_enabled"].as<bool>()) {
        if (root["goe_ctrl_hostname"].as<String>().length() == 0 || root["goe_ctrl_hostname"].as<String>().length() > INTEGRATIONS_GOE_MAX_HOSTNAME_STRLEN) {
            retMsg["message"] = "go-e Controller hostname must between 1 and " STR(INTEGRATIONS_GOE_MAX_HOSTNAME_STRLEN) " characters long!";
            retMsg["code"] = WebApiError::IntegrationsGoeControllerHostnameLength;
            retMsg["param"]["max"] = INTEGRATIONS_GOE_MAX_HOSTNAME_STRLEN;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }

        if (root["goe_ctrl_update_interval"].as<uint32_t>() < 3 || root["goe_ctrl_update_interval"].as<uint32_t>() > 65535) {
            retMsg["message"] = "go-e Controller update interval must between 3 and 65535!";
            retMsg["code"] = WebApiError::IntegrationsGoeControllerUpdateInterval;
            retMsg["param"]["min"] = 3;
            retMsg["param"]["max"] = 65535;
            WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
            return;
        }
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        config.Integrations.GoeControllerEnabled = root["goe_ctrl_enabled"].as<bool>();
        config.Integrations.GoeControllerPublishHomeCategory = root["goe_ctrl_publish_home_category"].as<bool>();
        config.Integrations.GoeControllerUpdateInterval = root["goe_ctrl_update_interval"].as<uint32_t>();
        strlcpy(config.Integrations.GoeControllerHostname, root["goe_ctrl_hostname"].as<String>().c_str(), sizeof(config.Integrations.GoeControllerHostname));
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

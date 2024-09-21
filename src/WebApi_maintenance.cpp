// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

#include "WebApi_maintenance.h"
#include "Utils.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>

void WebApiMaintenanceClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/maintenance/reboot", HTTP_POST, std::bind(&WebApiMaintenanceClass::onRebootPost, this, _1));
}

void WebApiMaintenanceClass::onRebootPost(AsyncWebServerRequest* request)
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

    if (!(root["reboot"].is<bool>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["reboot"].as<bool>()) {
        retMsg["type"] = "success";
        retMsg["message"] = "Reboot triggered!";
        retMsg["code"] = WebApiError::MaintenanceRebootTriggered;

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        Utils::restartDtu();
    } else {
        retMsg["message"] = "Reboot cancled!";
        retMsg["code"] = WebApiError::MaintenanceRebootCancled;

        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
    }
}

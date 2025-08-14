// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "WebApi_gridprofile.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiGridProfileClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/gridprofile/status", HTTP_GET, std::bind(&WebApiGridProfileClass::onGridProfileStatus, this, _1));
    server.on("/api/gridprofile/rawdata", HTTP_GET, std::bind(&WebApiGridProfileClass::onGridProfileRawdata, this, _1));
}

void WebApiGridProfileClass::onGridProfileStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto serial = WebApi.parseSerialFromRequest(request);
    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        root["name"] = inv->GridProfile()->getProfileName();
        root["version"] = inv->GridProfile()->getProfileVersion();

        auto jsonSections = root["sections"].to<JsonArray>();
        auto profSections = inv->GridProfile()->getProfile();

        for (auto& profSection : profSections) {
            auto jsonSection = jsonSections.add<JsonObject>();
            jsonSection["name"] = profSection.SectionName;

            auto jsonItems = jsonSection["items"].to<JsonArray>();

            for (auto& profItem : profSection.items) {
                auto jsonItem = jsonItems.add<JsonObject>();

                jsonItem["n"] = profItem.Name;
                jsonItem["u"] = profItem.Unit;
                jsonItem["v"] = profItem.Value;
            }
        }
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiGridProfileClass::onGridProfileRawdata(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto serial = WebApi.parseSerialFromRequest(request);
    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        auto raw = root["raw"].to<JsonArray>();
        auto data = inv->GridProfile()->getRawData();

        copyArray(&data[0], data.size(), raw);
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

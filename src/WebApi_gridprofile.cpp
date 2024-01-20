// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_gridprofile.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiGridProfileClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/gridprofile/status", HTTP_GET, std::bind(&WebApiGridProfileClass::onGridProfileStatus, this, _1));
    _server->on("/api/gridprofile/rawdata", HTTP_GET, std::bind(&WebApiGridProfileClass::onGridProfileRawdata, this, _1));
}

void WebApiGridProfileClass::onGridProfileStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 8192);
    auto& root = response->getRoot();

    uint64_t serial = 0;
    if (request->hasParam("inv")) {
        String s = request->getParam("inv")->value();
        serial = strtoll(s.c_str(), NULL, 16);
    }

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        root["name"] = inv->GridProfile()->getProfileName();
        root["version"] = inv->GridProfile()->getProfileVersion();

        auto jsonSections = root.createNestedArray("sections");
        auto profSections = inv->GridProfile()->getProfile();

        for (auto &profSection : profSections) {
            auto jsonSection = jsonSections.createNestedObject();
            jsonSection["name"] = profSection.SectionName;

            auto jsonItems = jsonSection.createNestedArray("items");

            for (auto &profItem : profSection.items) {
                auto jsonItem = jsonItems.createNestedObject();

                jsonItem["n"] = profItem.Name;
                jsonItem["u"] = profItem.Unit;
                jsonItem["v"] = profItem.Value;
            }
        }
    }

    response->setLength();
    request->send(response);
}

void WebApiGridProfileClass::onGridProfileRawdata(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096);
    auto& root = response->getRoot();

    uint64_t serial = 0;
    if (request->hasParam("inv")) {
        String s = request->getParam("inv")->value();
        serial = strtoll(s.c_str(), NULL, 16);
    }

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        auto raw = root.createNestedArray("raw");
        auto data = inv->GridProfile()->getRawData();

        copyArray(&data[0], data.size(), raw);
    }

    response->setLength();
    request->send(response);
}

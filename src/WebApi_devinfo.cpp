// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_devinfo.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>
#include <ctime>

void WebApiDevInfoClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/devinfo/status", HTTP_GET, std::bind(&WebApiDevInfoClass::onDevInfoStatus, this, _1));
}

void WebApiDevInfoClass::loop()
{
}

void WebApiDevInfoClass::onDevInfoStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    uint64_t serial = 0;
    if (request->hasParam("inv")) {
        String s = request->getParam("inv")->value();
        serial = strtoll(s.c_str(), NULL, 16);
    }

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        root["valid_data"] = inv->DevInfo()->getLastUpdate() > 0;
        root["fw_bootloader_version"] = inv->DevInfo()->getFwBootloaderVersion();
        root["fw_build_version"] = inv->DevInfo()->getFwBuildVersion();
        root["hw_part_number"] = inv->DevInfo()->getHwPartNumber();
        root["hw_version"] = inv->DevInfo()->getHwVersion();
        root["hw_model_name"] = inv->DevInfo()->getHwModelName();
        root["max_power"] = inv->DevInfo()->getMaxPower();

        char timebuffer[32];
        const time_t t = inv->DevInfo()->getFwBuildDateTime();
        std::strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
        root["fw_build_datetime"] = String(timebuffer);
    }

    response->setLength();
    request->send(response);
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_devinfo.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Hoymiles.h"
#include <ctime>

void WebApiDevInfoClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/devinfo/status", HTTP_GET, std::bind(&WebApiDevInfoClass::onDevInfoStatus, this, _1));
}

void WebApiDevInfoClass::loop()
{
}

void WebApiDevInfoClass::onDevInfoStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        // Inverter Serial is read as HEX
        char buffer[sizeof(uint64_t) * 8 + 1];
        snprintf(buffer, sizeof(buffer), "%0lx%08lx",
            ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

        JsonObject devInfoObj = root[buffer].createNestedObject();
        devInfoObj[F("valid_data")] = inv->DevInfo()->getLastUpdate() > 0;
        devInfoObj[F("fw_bootloader_version")] = inv->DevInfo()->getFwBootloaderVersion();
        devInfoObj[F("fw_build_version")] = inv->DevInfo()->getFwBuildVersion();
        devInfoObj[F("hw_part_number")] = inv->DevInfo()->getHwPartNumber();
        devInfoObj[F("hw_version")] = inv->DevInfo()->getHwVersion();

        char timebuffer[32];
        const time_t t = inv->DevInfo()->getFwBuildDateTime();
        std::strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
        devInfoObj[F("fw_build_datetime")] = String(timebuffer);
    }

    response->setLength();
    request->send(response);
}
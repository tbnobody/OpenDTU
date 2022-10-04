// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_power.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Hoymiles.h"

void WebApiPowerClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/power/status", HTTP_GET, std::bind(&WebApiPowerClass::onPowerStatus, this, _1));
    _server->on("/api/power/config", HTTP_POST, std::bind(&WebApiPowerClass::onPowerPost, this, _1));
}

void WebApiPowerClass::loop()
{
}

void WebApiPowerClass::onPowerStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        // Inverter Serial is read as HEX
        char buffer[sizeof(uint64_t) * 8 + 1];
        snprintf(buffer, sizeof(buffer), "%0x%08x",
            ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

        LastCommandSuccess status = inv->PowerCommand()->getLastPowerCommandSuccess();
        String limitStatus = "Unknown";
        if (status == LastCommandSuccess::CMD_OK) {
            limitStatus = "Ok";
        }
        else if (status == LastCommandSuccess::CMD_NOK) {
            limitStatus = "Failure";
        }
        else if (status == LastCommandSuccess::CMD_PENDING) {
            limitStatus = "Pending";
        }
        root[buffer]["power_set_status"] = limitStatus;
    }

    response->setLength();
    request->send(response);
}

void WebApiPowerClass::onPowerPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("serial")
            && root.containsKey("power"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        response->setLength();
        request->send(response);
        return;
    }

    uint64_t serial = strtoll(root[F("serial")].as<String>().c_str(), NULL, 16);
    uint16_t power = root[F("power")].as<bool>();

    auto inv = Hoymiles.getInverterBySerial(serial);
    if (inv == nullptr) {
        retMsg[F("message")] = F("Invalid inverter specified!");
        response->setLength();
        request->send(response);
        return;
    }

    inv->sendPowerControlRequest(Hoymiles.getRadio(), power);

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);
}
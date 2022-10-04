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

    _server->on("/api/power/config", HTTP_POST, std::bind(&WebApiPowerClass::onPowerPost, this, _1));
}

void WebApiPowerClass::loop()
{
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
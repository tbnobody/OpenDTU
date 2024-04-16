// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "Relay.h"
#include "PinMapping.h"
#include "WebApi_relay.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiRelayClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/relay/config", HTTP_GET, std::bind(&WebApiRelayClass::onRelayGet, this, _1));
    server.on("/api/relay/config", HTTP_POST, std::bind(&WebApiRelayClass::onRelayPost, this, _1));
    server.on("/api/relay/authenticate", HTTP_GET, std::bind(&WebApiRelayClass::onAuthenticateGet, this, _1));
}

void WebApiRelayClass::onRelayGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    const PinMapping_t& pin = PinMapping.get();
    Relay Cmd_Relay_R01 = Relay(pin.relay_r01);
    Relay Cmd_Relay_R02 = Relay(pin.relay_r02);

    root["R01"] = Cmd_Relay_R01.isStat();
    root["R02"] = Cmd_Relay_R02.isStat();
    root["PIN R01"] = pin.relay_r01;
    root["PIN R02"] = pin.relay_r02;


    response->setLength();
    request->send(response);
}

void WebApiRelayClass::onRelayPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    //retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    const String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    const DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("R01") || root.containsKey("R02"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["R01"].as<uint8_t>() > 1) {
        retMsg["message"] = "Invalid relay R01 setting!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["R02"].as<uint8_t>() > 1) {
        retMsg["message"] = "Invalid relay R02 setting!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    const auto& pin = PinMapping.get();
    Relay Cmd_Relay_R01 = Relay(pin.relay_r01);
    Relay Cmd_Relay_R02 = Relay(pin.relay_r02);

    if (root["R01"].as<uint8_t>() == 1) {
        Cmd_Relay_R01.on();
    }
    else {
        Cmd_Relay_R01.off();
    }

    if (root["R02"].as<uint8_t>() == 1) {
        Cmd_Relay_R02.on();
    }
    else {
        Cmd_Relay_R02.off();
    }

    response->setLength();
    request->send(response);
}

void WebApiRelayClass::onAuthenticateGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    retMsg["type"] = "success";
    retMsg["message"] = "Authentication successful!";

    response->setLength();
    request->send(response);
}

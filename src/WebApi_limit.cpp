// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_limit.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "defaults.h"
#include "helper.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiLimitClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/limit/status", HTTP_GET, std::bind(&WebApiLimitClass::onLimitStatus, this, _1));
    _server->on("/api/limit/config", HTTP_POST, std::bind(&WebApiLimitClass::onLimitPost, this, _1));
}

void WebApiLimitClass::loop()
{
}

void WebApiLimitClass::onLimitStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        String serial = inv->serialString();

        root[serial]["limit_relative"] = inv->SystemConfigPara()->getLimitPercent();
        root[serial]["max_power"] = inv->DevInfo()->getMaxPower();

        LastCommandSuccess status = inv->SystemConfigPara()->getLastLimitCommandSuccess();
        String limitStatus = "Unknown";
        if (status == LastCommandSuccess::CMD_OK) {
            limitStatus = "Ok";
        } else if (status == LastCommandSuccess::CMD_NOK) {
            limitStatus = "Failure";
        } else if (status == LastCommandSuccess::CMD_PENDING) {
            limitStatus = "Pending";
        }
        root[serial]["limit_set_status"] = limitStatus;
    }

    response->setLength();
    request->send(response);
}

void WebApiLimitClass::onLimitPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

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

    if (!(root.containsKey("serial")
            && root.containsKey("limit_value")
            && root.containsKey("limit_type"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["serial"].as<uint64_t>() == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::LimitSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["limit_value"].as<uint16_t>() > MAX_INVERTER_LIMIT) {
        retMsg["message"] = "Limit must between 0 and " STR(MAX_INVERTER_LIMIT) "!";
        retMsg["code"] = WebApiError::LimitInvalidLimit;
        retMsg["param"]["max"] = MAX_INVERTER_LIMIT;
        response->setLength();
        request->send(response);
        return;
    }

    if (!((root["limit_type"].as<uint16_t>() == PowerLimitControlType::AbsolutNonPersistent)
            || (root["limit_type"].as<uint16_t>() == PowerLimitControlType::AbsolutPersistent)
            || (root["limit_type"].as<uint16_t>() == PowerLimitControlType::RelativNonPersistent)
            || (root["limit_type"].as<uint16_t>() == PowerLimitControlType::RelativPersistent))) {

        retMsg["message"] = "Invalid type specified!";
        retMsg["code"] = WebApiError::LimitInvalidType;
        response->setLength();
        request->send(response);
        return;
    }

    uint64_t serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);
    uint16_t limit = root["limit_value"].as<uint16_t>();
    PowerLimitControlType type = root["limit_type"].as<PowerLimitControlType>();

    auto inv = Hoymiles.getInverterBySerial(serial);
    if (inv == nullptr) {
        retMsg["message"] = "Invalid inverter specified!";
        retMsg["code"] = WebApiError::LimitInvalidInverter;
        response->setLength();
        request->send(response);
        return;
    }

    inv->sendActivePowerControlRequest(limit, type);

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}
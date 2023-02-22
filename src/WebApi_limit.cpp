// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_limit.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "Failsafe.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiLimitClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

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
    JsonObject root = response->getRoot();

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
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("serial")
            && root.containsKey("limit_value")
            && root.containsKey("limit_type"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        retMsg[F("code")] = WebApiError::LimitSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("limit_value")].as<uint16_t>() == 0 || root[F("limit_value")].as<uint16_t>() > 1500) {
        retMsg[F("message")] = F("Limit must between 1 and 1500!");
        retMsg[F("code")] = WebApiError::LimitInvalidLimit;
        retMsg[F("param")][F("max")] = 1500;
        response->setLength();
        request->send(response);
        return;
    }

    if (!((root[F("limit_type")].as<uint16_t>() == PowerLimitControlType::AbsolutNonPersistent)
            || (root[F("limit_type")].as<uint16_t>() == PowerLimitControlType::AbsolutPersistent)
            || (root[F("limit_type")].as<uint16_t>() == PowerLimitControlType::RelativNonPersistent)
            || (root[F("limit_type")].as<uint16_t>() == PowerLimitControlType::RelativPersistent))) {

        retMsg[F("message")] = F("Invalid type specified!");
        retMsg[F("code")] = WebApiError::LimitInvalidType;
        response->setLength();
        request->send(response);
        return;
    }

    uint64_t serial = strtoll(root[F("serial")].as<String>().c_str(), NULL, 16);
    uint16_t limit = root[F("limit_value")].as<uint16_t>();
    PowerLimitControlType type = root[F("limit_type")].as<PowerLimitControlType>();

    auto inv = Hoymiles.getInverterBySerial(serial);
    if (inv == nullptr) {
        retMsg[F("message")] = F("Invalid inverter specified!");
        retMsg[F("code")] = WebApiError::LimitInvalidInverter;
        response->setLength();
        request->send(response);
        return;
    }

    FailsafeCheck.RequestReceived(Hoymiles.getInverterPosBySerial(serial));
    inv->sendActivePowerControlRequest(Hoymiles.getRadio(), limit, type);

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}
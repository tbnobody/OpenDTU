// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_dtu.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiDtuClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/dtu/config", HTTP_GET, std::bind(&WebApiDtuClass::onDtuAdminGet, this, _1));
    _server->on("/api/dtu/config", HTTP_POST, std::bind(&WebApiDtuClass::onDtuAdminPost, this, _1));
}

void WebApiDtuClass::loop()
{
}

void WebApiDtuClass::onDtuAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    // DTU Serial is read as HEX
    char buffer[sizeof(uint64_t) * 8 + 1];
    snprintf(buffer, sizeof(buffer), "%0x%08x",
        ((uint32_t)((config.Dtu_Serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(config.Dtu_Serial & 0xFFFFFFFF)));
    root["serial"] = buffer;
    root["pollinterval"] = config.Dtu_PollInterval;
    root["nrf_enabled"] = Hoymiles.getRadioNrf()->isInitialized();
    root["nrf_palevel"] = config.Dtu_NrfPaLevel;
    root["cmt_enabled"] = Hoymiles.getRadioCmt()->isInitialized();
    root["cmt_palevel"] = config.Dtu_CmtPaLevel;
    root["cmt_frequency"] = config.Dtu_CmtFrequency;

    response->setLength();
    request->send(response);
}

void WebApiDtuClass::onDtuAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("serial") && root.containsKey("pollinterval") && root.containsKey("nrf_palevel") && root.containsKey("cmt_palevel") && root.containsKey("cmt_frequency"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["serial"].as<uint64_t>() == 0) {
        retMsg["message"] = "Serial cannot be zero!";
        retMsg["code"] = WebApiError::DtuSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["pollinterval"].as<uint32_t>() == 0) {
        retMsg["message"] = "Poll interval must be greater zero!";
        retMsg["code"] = WebApiError::DtuPollZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["nrf_palevel"].as<uint8_t>() > 3) {
        retMsg["message"] = "Invalid power level setting!";
        retMsg["code"] = WebApiError::DtuInvalidPowerLevel;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["cmt_palevel"].as<int8_t>() < -10 || root["cmt_palevel"].as<int8_t>() > 20) {
        retMsg["message"] = "Invalid power level setting!";
        retMsg["code"] = WebApiError::DtuInvalidPowerLevel;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["cmt_frequency"].as<uint32_t>() < Hoymiles.getRadioCmt()->getMinFrequency()
        || root["cmt_frequency"].as<uint32_t>() > Hoymiles.getRadioCmt()->getMaxFrequency()
        || root["cmt_frequency"].as<uint32_t>() % 250 > 0) {

        retMsg["message"] = "Invalid CMT frequency setting!";
        retMsg["code"] = WebApiError::DtuInvalidCmtFrequency;
        retMsg["param"]["min"] = Hoymiles.getRadioCmt()->getMinFrequency();
        retMsg["param"]["max"] = Hoymiles.getRadioCmt()->getMaxFrequency();
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();

    // Interpret the string as a hex value and convert it to uint64_t
    config.Dtu_Serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);
    config.Dtu_PollInterval = root["pollinterval"].as<uint32_t>();
    config.Dtu_NrfPaLevel = root["nrf_palevel"].as<uint8_t>();
    config.Dtu_CmtPaLevel = root["cmt_palevel"].as<int8_t>();
    config.Dtu_CmtFrequency = root["cmt_frequency"].as<uint32_t>();
    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    Hoymiles.getRadioNrf()->setPALevel((rf24_pa_dbm_e)config.Dtu_NrfPaLevel);
    Hoymiles.getRadioCmt()->setPALevel(config.Dtu_CmtPaLevel);
    Hoymiles.getRadioNrf()->setDtuSerial(config.Dtu_Serial);
    Hoymiles.getRadioCmt()->setDtuSerial(config.Dtu_Serial);
    Hoymiles.getRadioCmt()->setInverterTargetFrequency(config.Dtu_CmtFrequency);
    Hoymiles.setPollInterval(config.Dtu_PollInterval);
}
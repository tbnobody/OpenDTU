// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_dtu.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

WebApiDtuClass::WebApiDtuClass()
    : _applyDataTask(TASK_IMMEDIATE, TASK_ONCE, std::bind(&WebApiDtuClass::applyDataTaskCb, this))
{
}

void WebApiDtuClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/dtu/config", HTTP_GET, std::bind(&WebApiDtuClass::onDtuAdminGet, this, _1));
    server.on("/api/dtu/config", HTTP_POST, std::bind(&WebApiDtuClass::onDtuAdminPost, this, _1));

    scheduler.addTask(_applyDataTask);
}

void WebApiDtuClass::applyDataTaskCb()
{
    // Execute stuff in main thread to avoid busy SPI bus
    CONFIG_T& config = Configuration.get();
    Hoymiles.getRadioNrf()->setPALevel((rf24_pa_dbm_e)config.Dtu.Nrf.PaLevel);
    Hoymiles.getRadioCmt()->setPALevel(config.Dtu.Cmt.PaLevel);
    Hoymiles.getRadioNrf()->setDtuSerial(config.Dtu.Serial);
    Hoymiles.getRadioCmt()->setDtuSerial(config.Dtu.Serial);
    Hoymiles.getRadioCmt()->setCountryMode(static_cast<CountryModeId_t>(config.Dtu.Cmt.CountryMode));
    Hoymiles.getRadioCmt()->setInverterTargetFrequency(config.Dtu.Cmt.Frequency);
    Hoymiles.setPollInterval(config.Dtu.PollInterval);
}

void WebApiDtuClass::onDtuAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    // DTU Serial is read as HEX
    char buffer[sizeof(uint64_t) * 8 + 1];
    snprintf(buffer, sizeof(buffer), "%0x%08x",
        ((uint32_t)((config.Dtu.Serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(config.Dtu.Serial & 0xFFFFFFFF)));
    root["serial"] = buffer;
    root["pollinterval"] = config.Dtu.PollInterval;
    root["nrf_enabled"] = Hoymiles.getRadioNrf()->isInitialized();
    root["nrf_palevel"] = config.Dtu.Nrf.PaLevel;
    root["cmt_enabled"] = Hoymiles.getRadioCmt()->isInitialized();
    root["cmt_palevel"] = config.Dtu.Cmt.PaLevel;
    root["cmt_frequency"] = config.Dtu.Cmt.Frequency;
    root["cmt_country"] = config.Dtu.Cmt.CountryMode;
    root["cmt_chan_width"] = Hoymiles.getRadioCmt()->getChannelWidth();

    auto data = root["country_def"].to<JsonArray>();
    auto countryDefs = Hoymiles.getRadioCmt()->getCountryFrequencyList();
    for (const auto& definition : countryDefs) {
        auto obj = data.add<JsonObject>();
        obj["freq_default"] = definition.definition.Freq_Default;
        obj["freq_min"] = definition.definition.Freq_Min;
        obj["freq_max"] = definition.definition.Freq_Max;
        obj["freq_legal_min"] = definition.definition.Freq_Legal_Min;
        obj["freq_legal_max"] = definition.definition.Freq_Legal_Max;
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiDtuClass::onDtuAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["serial"].is<String>()
            && root["pollinterval"].is<uint32_t>()
            && root["nrf_palevel"].is<uint8_t>()
            && root["cmt_palevel"].is<int8_t>()
            && root["cmt_frequency"].is<uint32_t>()
            && root["cmt_country"].is<uint8_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    const uint64_t serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);

    if (serial == 0) {
        retMsg["message"] = "Serial cannot be zero!";
        retMsg["code"] = WebApiError::DtuSerialZero;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["pollinterval"].as<uint32_t>() == 0) {
        retMsg["message"] = "Poll interval must be greater zero!";
        retMsg["code"] = WebApiError::DtuPollZero;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["nrf_palevel"].as<uint8_t>() > 3) {
        retMsg["message"] = "Invalid power level setting!";
        retMsg["code"] = WebApiError::DtuInvalidPowerLevel;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["cmt_palevel"].as<int8_t>() < -10 || root["cmt_palevel"].as<int8_t>() > 20) {
        retMsg["message"] = "Invalid power level setting!";
        retMsg["code"] = WebApiError::DtuInvalidPowerLevel;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["cmt_country"].as<uint8_t>() >= CountryModeId_t::CountryModeId_Max) {
        retMsg["message"] = "Invalid country setting!";
        retMsg["code"] = WebApiError::DtuInvalidCmtCountry;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto FrequencyDefinition = Hoymiles.getRadioCmt()->getCountryFrequencyList()[root["cmt_country"].as<CountryModeId_t>()].definition;
    if (root["cmt_frequency"].as<uint32_t>() < FrequencyDefinition.Freq_Min
        || root["cmt_frequency"].as<uint32_t>() > FrequencyDefinition.Freq_Max
        || root["cmt_frequency"].as<uint32_t>() % Hoymiles.getRadioCmt()->getChannelWidth() > 0) {

        retMsg["message"] = "Invalid CMT frequency setting!";
        retMsg["code"] = WebApiError::DtuInvalidCmtFrequency;
        retMsg["param"]["min"] = FrequencyDefinition.Freq_Min;
        retMsg["param"]["max"] = FrequencyDefinition.Freq_Max;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    CONFIG_T& config = Configuration.get();

    config.Dtu.Serial = serial;
    config.Dtu.PollInterval = root["pollinterval"].as<uint32_t>();
    config.Dtu.Nrf.PaLevel = root["nrf_palevel"].as<uint8_t>();
    config.Dtu.Cmt.PaLevel = root["cmt_palevel"].as<int8_t>();
    config.Dtu.Cmt.Frequency = root["cmt_frequency"].as<uint32_t>();
    config.Dtu.Cmt.CountryMode = root["cmt_country"].as<CountryModeId_t>();

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    _applyDataTask.enable();
    _applyDataTask.restart();
}

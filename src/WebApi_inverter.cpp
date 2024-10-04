// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_inverter.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "defaults.h"
#include "helper.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiInverterClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/inverter/list", HTTP_GET, std::bind(&WebApiInverterClass::onInverterList, this, _1));
    server.on("/api/inverter/add", HTTP_POST, std::bind(&WebApiInverterClass::onInverterAdd, this, _1));
    server.on("/api/inverter/edit", HTTP_POST, std::bind(&WebApiInverterClass::onInverterEdit, this, _1));
    server.on("/api/inverter/del", HTTP_POST, std::bind(&WebApiInverterClass::onInverterDelete, this, _1));
    server.on("/api/inverter/order", HTTP_POST, std::bind(&WebApiInverterClass::onInverterOrder, this, _1));
    server.on("/api/inverter/stats_reset", HTTP_GET, std::bind(&WebApiInverterClass::onInverterStatReset, this, _1));
}

void WebApiInverterClass::onInverterList(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    JsonArray data = root["inverter"].to<JsonArray>();

    const CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            JsonObject obj = data.add<JsonObject>();
            obj["id"] = i;
            obj["name"] = String(config.Inverter[i].Name);
            obj["order"] = config.Inverter[i].Order;

            // Inverter Serial is read as HEX
            char buffer[sizeof(uint64_t) * 8 + 1];
            snprintf(buffer, sizeof(buffer), "%0" PRIx32 "%08" PRIx32,
                ((uint32_t)((config.Inverter[i].Serial >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(config.Inverter[i].Serial & 0xFFFFFFFF)));
            obj["serial"] = buffer;
            obj["poll_enable"] = config.Inverter[i].Poll_Enable;
            obj["poll_enable_night"] = config.Inverter[i].Poll_Enable_Night;
            obj["command_enable"] = config.Inverter[i].Command_Enable;
            obj["command_enable_night"] = config.Inverter[i].Command_Enable_Night;
            obj["reachable_threshold"] = config.Inverter[i].ReachableThreshold;
            obj["zero_runtime"] = config.Inverter[i].ZeroRuntimeDataIfUnrechable;
            obj["zero_day"] = config.Inverter[i].ZeroYieldDayOnMidnight;
            obj["clear_eventlog"] = config.Inverter[i].ClearEventlogOnMidnight;
            obj["yieldday_correction"] = config.Inverter[i].YieldDayCorrection;

            auto inv = Hoymiles.getInverterBySerial(config.Inverter[i].Serial);
            uint8_t max_channels;
            if (inv == nullptr) {
                obj["type"] = "Unknown";
                max_channels = INV_MAX_CHAN_COUNT;
            } else {
                obj["type"] = inv->typeName();
                max_channels = inv->Statistics()->getChannelsByType(TYPE_DC).size();
            }

            JsonArray channel = obj["channel"].to<JsonArray>();
            for (uint8_t c = 0; c < max_channels; c++) {
                JsonObject chanData = channel.add<JsonObject>();
                chanData["name"] = config.Inverter[i].channel[c].Name;
                chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
                chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
            }
        }
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiInverterClass::onInverterAdd(AsyncWebServerRequest* request)
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
            && root["name"].is<String>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    const uint64_t serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);

    if (serial == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::InverterSerialZero;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::InverterNameLength;
        retMsg["param"]["max"] = INV_MAX_NAME_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    INVERTER_CONFIG_T* inverter = Configuration.getFreeInverterSlot();

    if (!inverter) {
        retMsg["message"] = "Only " STR(INV_MAX_COUNT) " inverters are supported!";
        retMsg["code"] = WebApiError::InverterCount;
        retMsg["param"]["max"] = INV_MAX_COUNT;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    inverter->Serial = serial;

    strncpy(inverter->Name, root["name"].as<String>().c_str(), INV_MAX_NAME_STRLEN);

    WebApi.writeConfig(retMsg, WebApiError::InverterAdded, "Inverter created!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    auto inv = Hoymiles.addInverter(inverter->Name, inverter->Serial);

    if (inv != nullptr) {
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            inv->Statistics()->setStringMaxPower(c, inverter->channel[c].MaxChannelPower);
        }
    }

    MqttHandleHass.forceUpdate();
}

void WebApiInverterClass::onInverterEdit(AsyncWebServerRequest* request)
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

    if (!(root["id"].is<uint8_t>()
            && root["serial"].is<String>()
            && root["name"].is<String>()
            && root["channel"].is<JsonArray>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["id"].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::InverterInvalidId;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    const uint64_t serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);

    if (serial == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::InverterSerialZero;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::InverterNameLength;
        retMsg["param"]["max"] = INV_MAX_NAME_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    JsonArray channelArray = root["channel"].as<JsonArray>();
    if (channelArray.size() == 0 || channelArray.size() > INV_MAX_CHAN_COUNT) {
        retMsg["message"] = "Invalid amount of max channel setting given!";
        retMsg["code"] = WebApiError::InverterInvalidMaxChannel;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[root["id"].as<uint8_t>()];

    uint64_t new_serial = serial;
    uint64_t old_serial = inverter.Serial;

    // Interpret the string as a hex value and convert it to uint64_t
    inverter.Serial = new_serial;
    strncpy(inverter.Name, root["name"].as<String>().c_str(), INV_MAX_NAME_STRLEN);

    inverter.Poll_Enable = root["poll_enable"] | true;
    inverter.Poll_Enable_Night = root["poll_enable_night"] | true;
    inverter.Command_Enable = root["command_enable"] | true;
    inverter.Command_Enable_Night = root["command_enable_night"] | true;
    inverter.ReachableThreshold = root["reachable_threshold"] | REACHABLE_THRESHOLD;
    inverter.ZeroRuntimeDataIfUnrechable = root["zero_runtime"] | false;
    inverter.ZeroYieldDayOnMidnight = root["zero_day"] | false;
    inverter.ClearEventlogOnMidnight = root["clear_eventlog"] | false;
    inverter.YieldDayCorrection = root["yieldday_correction"] | false;

    uint8_t arrayCount = 0;
    for (JsonVariant channel : channelArray) {
        inverter.channel[arrayCount].MaxChannelPower = channel["max_power"].as<uint16_t>();
        inverter.channel[arrayCount].YieldTotalOffset = channel["yield_total_offset"].as<float>();
        strncpy(inverter.channel[arrayCount].Name, channel["name"] | "", sizeof(inverter.channel[arrayCount].Name));
        arrayCount++;
    }

    WebApi.writeConfig(retMsg, WebApiError::InverterChanged, "Inverter changed!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(old_serial);

    if (inv != nullptr && new_serial != old_serial) {
        // Valid inverter exists but serial changed --> remove it and insert new one
        Hoymiles.removeInverterBySerial(old_serial);
        inv = Hoymiles.addInverter(inverter.Name, inverter.Serial);
    } else if (inv != nullptr && new_serial == old_serial) {
        // Valid inverter exists and serial stays the same --> update name
        inv->setName(inverter.Name);
    } else if (inv == nullptr) {
        // Valid inverter did not exist --> try to create one
        inv = Hoymiles.addInverter(inverter.Name, inverter.Serial);
    }

    if (inv != nullptr) {
        inv->setEnablePolling(inverter.Poll_Enable);
        inv->setEnableCommands(inverter.Command_Enable);
        inv->setReachableThreshold(inverter.ReachableThreshold);
        inv->setZeroValuesIfUnreachable(inverter.ZeroRuntimeDataIfUnrechable);
        inv->setZeroYieldDayOnMidnight(inverter.ZeroYieldDayOnMidnight);
        inv->setClearEventlogOnMidnight(inverter.ClearEventlogOnMidnight);
        inv->Statistics()->setYieldDayCorrection(inverter.YieldDayCorrection);
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            inv->Statistics()->setStringMaxPower(c, inverter.channel[c].MaxChannelPower);
            inv->Statistics()->setChannelFieldOffset(TYPE_DC, static_cast<ChannelNum_t>(c), FLD_YT, inverter.channel[c].YieldTotalOffset);
        }
    }

    MqttHandleHass.forceUpdate();
}

void WebApiInverterClass::onInverterDelete(AsyncWebServerRequest* request)
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

    if (!(root["id"].is<uint8_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["id"].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::InverterInvalidId;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    uint8_t inverter_id = root["id"].as<uint8_t>();
    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[inverter_id];

    Hoymiles.removeInverterBySerial(inverter.Serial);

    Configuration.deleteInverterById(inverter_id);

    WebApi.writeConfig(retMsg, WebApiError::InverterDeleted, "Inverter deleted!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    MqttHandleHass.forceUpdate();
}

void WebApiInverterClass::onInverterOrder(AsyncWebServerRequest* request)
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

    if (!(root["order"].is<JsonArray>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // The order array contains list or id in the right order
    JsonArray orderArray = root["order"].as<JsonArray>();
    uint8_t order = 0;
    for (JsonVariant id : orderArray) {
        uint8_t inverter_id = id.as<uint8_t>();
        if (inverter_id < INV_MAX_COUNT) {
            INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[inverter_id];
            inverter.Order = order;
        }
        order++;
    }

    WebApi.writeConfig(retMsg, WebApiError::InverterOrdered, "Inverter order saved!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiInverterClass::onInverterStatReset(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto retMsg = response->getRoot();
    auto serial = WebApi.parseSerialFromRequest(request);
    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        inv->resetRadioStats();
        retMsg["type"] = "success";
        retMsg["message"] = "Stats resetted";
        retMsg["code"] = WebApiError::InverterStatsResetted;
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

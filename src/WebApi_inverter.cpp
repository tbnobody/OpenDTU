// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
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

void WebApiInverterClass::init(AsyncWebServer& server)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/inverter/list", HTTP_GET, std::bind(&WebApiInverterClass::onInverterList, this, _1));
    _server->on("/api/inverter/add", HTTP_POST, std::bind(&WebApiInverterClass::onInverterAdd, this, _1));
    _server->on("/api/inverter/edit", HTTP_POST, std::bind(&WebApiInverterClass::onInverterEdit, this, _1));
    _server->on("/api/inverter/del", HTTP_POST, std::bind(&WebApiInverterClass::onInverterDelete, this, _1));
    _server->on("/api/inverter/order", HTTP_POST, std::bind(&WebApiInverterClass::onInverterOrder, this, _1));
}

void WebApiInverterClass::loop()
{
}

void WebApiInverterClass::onInverterList(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 768 * INV_MAX_COUNT);
    JsonObject root = response->getRoot();
    JsonArray data = root.createNestedArray("inverter");

    const CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            JsonObject obj = data.createNestedObject();
            obj["id"] = i;
            obj["name"] = String(config.Inverter[i].Name);
            obj["order"] = config.Inverter[i].Order;

            // Inverter Serial is read as HEX
            char buffer[sizeof(uint64_t) * 8 + 1];
            snprintf(buffer, sizeof(buffer), "%0x%08x",
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

            JsonArray channel = obj.createNestedArray("channel");
            for (uint8_t c = 0; c < max_channels; c++) {
                JsonObject chanData = channel.createNestedObject();
                chanData["name"] = config.Inverter[i].channel[c].Name;
                chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
                chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
            }
        }
    }

    response->setLength();
    request->send(response);
}

void WebApiInverterClass::onInverterAdd(AsyncWebServerRequest* request)
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
            && root.containsKey("name"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["serial"].as<uint64_t>() == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::InverterSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::InverterNameLength;
        retMsg["param"]["max"] = INV_MAX_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T* inverter = Configuration.getFreeInverterSlot();

    if (!inverter) {
        retMsg["message"] = "Only " STR(INV_MAX_COUNT) " inverters are supported!";
        retMsg["code"] = WebApiError::InverterCount;
        retMsg["param"]["max"] = INV_MAX_COUNT;
        response->setLength();
        request->send(response);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    inverter->Serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);

    strncpy(inverter->Name, root["name"].as<String>().c_str(), INV_MAX_NAME_STRLEN);
    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Inverter created!";
    retMsg["code"] = WebApiError::InverterAdded;

    response->setLength();
    request->send(response);

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
    JsonObject retMsg = response->getRoot();
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

    if (!(root.containsKey("id") && root.containsKey("serial") && root.containsKey("name") && root.containsKey("channel"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["id"].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::InverterInvalidId;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["serial"].as<uint64_t>() == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::InverterSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::InverterNameLength;
        retMsg["param"]["max"] = INV_MAX_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    JsonArray channelArray = root["channel"].as<JsonArray>();
    if (channelArray.size() == 0 || channelArray.size() > INV_MAX_CHAN_COUNT) {
        retMsg["message"] = "Invalid amount of max channel setting given!";
        retMsg["code"] = WebApiError::InverterInvalidMaxChannel;
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[root["id"].as<uint8_t>()];

    uint64_t new_serial = strtoll(root["serial"].as<String>().c_str(), NULL, 16);
    uint64_t old_serial = inverter.Serial;

    // Interpret the string as a hex value and convert it to uint64_t
    inverter.Serial = new_serial;
    strncpy(inverter.Name, root["name"].as<String>().c_str(), INV_MAX_NAME_STRLEN);

    uint8_t arrayCount = 0;
    for (JsonVariant channel : channelArray) {
        inverter.channel[arrayCount].MaxChannelPower = channel["max_power"].as<uint16_t>();
        inverter.channel[arrayCount].YieldTotalOffset = channel["yield_total_offset"].as<float>();
        strncpy(inverter.channel[arrayCount].Name, channel["name"] | "", sizeof(inverter.channel[arrayCount].Name));
        inverter.Poll_Enable = root["poll_enable"] | true;
        inverter.Poll_Enable_Night = root["poll_enable_night"] | true;
        inverter.Command_Enable = root["command_enable"] | true;
        inverter.Command_Enable_Night = root["command_enable_night"] | true;
        inverter.ReachableThreshold = root["reachable_threshold"] | REACHABLE_THRESHOLD;
        inverter.ZeroRuntimeDataIfUnrechable = root["zero_runtime"] | false;
        inverter.ZeroYieldDayOnMidnight = root["zero_day"] | false;
        inverter.YieldDayCorrection = root["yieldday_correction"] | false;

        arrayCount++;
    }

    Configuration.write();

    retMsg["type"] = "success";
    retMsg["code"] = WebApiError::InverterChanged;
    retMsg["message"] = "Inverter changed!";

    response->setLength();
    request->send(response);

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
    JsonObject retMsg = response->getRoot();
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

    if (!(root.containsKey("id"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["id"].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::InverterInvalidId;
        response->setLength();
        request->send(response);
        return;
    }

    uint8_t inverter_id = root["id"].as<uint8_t>();
    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[inverter_id];

    Hoymiles.removeInverterBySerial(inverter.Serial);

    inverter.Serial = 0;
    strncpy(inverter.Name, "", sizeof(inverter.Name));
    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Inverter deleted!";
    retMsg["code"] = WebApiError::InverterDeleted;

    response->setLength();
    request->send(response);

    MqttHandleHass.forceUpdate();
}

void WebApiInverterClass::onInverterOrder(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("order"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
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

    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Inverter order saved!";
    retMsg["code"] = WebApiError::InverterOrdered;

    response->setLength();
    request->send(response);
}
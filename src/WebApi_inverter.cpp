// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_inverter.h"
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiInverterClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/inverter/list", HTTP_GET, std::bind(&WebApiInverterClass::onInverterList, this, _1));
    _server->on("/api/inverter/add", HTTP_POST, std::bind(&WebApiInverterClass::onInverterAdd, this, _1));
    _server->on("/api/inverter/edit", HTTP_POST, std::bind(&WebApiInverterClass::onInverterEdit, this, _1));
    _server->on("/api/inverter/del", HTTP_POST, std::bind(&WebApiInverterClass::onInverterDelete, this, _1));
}

void WebApiInverterClass::loop()
{
}

void WebApiInverterClass::onInverterList(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096U);
    JsonObject root = response->getRoot();
    JsonArray data = root.createNestedArray(F("inverter"));

    const CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            JsonObject obj = data.createNestedObject();
            obj[F("id")] = i;
            obj[F("name")] = String(config.Inverter[i].Name);

            // Inverter Serial is read as HEX
            char buffer[sizeof(uint64_t) * 8 + 1];
            snprintf(buffer, sizeof(buffer), "%0x%08x",
                ((uint32_t)((config.Inverter[i].Serial >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(config.Inverter[i].Serial & 0xFFFFFFFF)));
            obj[F("serial")] = buffer;

            auto inv = Hoymiles.getInverterBySerial(config.Inverter[i].Serial);
            uint8_t max_channels;
            if (inv == nullptr) {
                obj[F("type")] = F("Unknown");
                max_channels = INV_MAX_CHAN_COUNT;
            } else {
                obj[F("type")] = inv->typeName();
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

    if (!(root.containsKey("serial") && root.containsKey("name"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        retMsg[F("code")] = WebApiError::InverterSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("name")].as<String>().length() == 0 || root[F("name")].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg[F("message")] = F("Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::InverterNameLength;
        retMsg[F("param")][F("max")] = INV_MAX_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T* inverter = Configuration.getFreeInverterSlot();

    if (!inverter) {
        retMsg[F("message")] = F("Only " STR(INV_MAX_COUNT) " inverters are supported!");
        retMsg[F("code")] = WebApiError::InverterCount;
        retMsg[F("param")][F("max")] = INV_MAX_COUNT;
        response->setLength();
        request->send(response);
        return;
    }

    // Interpret the string as a hex value and convert it to uint64_t
    inverter->Serial = strtoll(root[F("serial")].as<String>().c_str(), NULL, 16);

    strncpy(inverter->Name, root[F("name")].as<String>().c_str(), INV_MAX_NAME_STRLEN);
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Inverter created!");
    retMsg[F("code")] = WebApiError::InverterAdded;

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

    if (!(root.containsKey("id") && root.containsKey("serial") && root.containsKey("name") && root.containsKey("channel"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("id")].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg[F("message")] = F("Invalid ID specified!");
        retMsg[F("code")] = WebApiError::InverterInvalidId;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        retMsg[F("code")] = WebApiError::InverterSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("name")].as<String>().length() == 0 || root[F("name")].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg[F("message")] = F("Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::InverterNameLength;
        retMsg[F("param")][F("max")] = INV_MAX_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    JsonArray channelArray = root[F("channel")].as<JsonArray>();
    if (channelArray.size() == 0 || channelArray.size() > INV_MAX_CHAN_COUNT) {
        retMsg[F("message")] = F("Invalid amount of max channel setting given!");
        retMsg[F("code")] = WebApiError::InverterInvalidMaxChannel;
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[root[F("id")].as<uint8_t>()];

    uint64_t new_serial = strtoll(root[F("serial")].as<String>().c_str(), NULL, 16);
    uint64_t old_serial = inverter.Serial;

    // Interpret the string as a hex value and convert it to uint64_t
    inverter.Serial = new_serial;
    strncpy(inverter.Name, root[F("name")].as<String>().c_str(), INV_MAX_NAME_STRLEN);

    uint8_t arrayCount = 0;
    for (JsonVariant channel : channelArray) {
        inverter.channel[arrayCount].MaxChannelPower = channel[F("max_power")].as<uint16_t>();
        inverter.channel[arrayCount].YieldTotalOffset = channel[F("yield_total_offset")].as<float>();
        strncpy(inverter.channel[arrayCount].Name, channel[F("name")] | "", sizeof(inverter.channel[arrayCount].Name));
        arrayCount++;
    }

    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("code")] = WebApiError::InverterChanged;
    retMsg[F("message")] = F("Inverter changed!");

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

    if (!(root.containsKey("id"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("id")].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg[F("message")] = F("Invalid ID specified!");
        retMsg[F("code")] = WebApiError::InverterInvalidId;
        response->setLength();
        request->send(response);
        return;
    }

    uint8_t inverter_id = root[F("id")].as<uint8_t>();
    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[inverter_id];

    Hoymiles.removeInverterBySerial(inverter.Serial);

    inverter.Serial = 0;
    strncpy(inverter.Name, "", sizeof(inverter.Name));
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Inverter deleted!");
    retMsg[F("code")] = WebApiError::InverterDeleted;

    response->setLength();
    request->send(response);

    MqttHandleHass.forceUpdate();
}
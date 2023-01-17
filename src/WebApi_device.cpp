// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_device.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiDeviceClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/device/config", HTTP_GET, std::bind(&WebApiDeviceClass::onDeviceAdminGet, this, _1));
    _server->on("/api/device/config", HTTP_POST, std::bind(&WebApiDeviceClass::onDeviceAdminPost, this, _1));
}

void WebApiDeviceClass::loop()
{
}

void WebApiDeviceClass::onDeviceAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, MQTT_JSON_DOC_SIZE);
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();
    const PinMapping_t& pin = PinMapping.get();

    JsonObject curPin = root.createNestedObject("curPin");
    curPin[F("name")] = config.Dev_PinMapping;

    JsonObject nrfObj = curPin.createNestedObject("nrf24");
    nrfObj[F("clk")] = pin.nrf24_clk;
    nrfObj[F("cs")] = pin.nrf24_cs;
    nrfObj[F("en")] = pin.nrf24_en;
    nrfObj[F("irq")] = pin.nrf24_irq;
    nrfObj[F("miso")] = pin.nrf24_miso;
    nrfObj[F("mosi")] = pin.nrf24_mosi;

    JsonObject ethObj = curPin.createNestedObject("eth");
    ethObj[F("enabled")] = pin.eth_enabled;
    ethObj[F("phy_addr")] = pin.eth_phy_addr;
    ethObj[F("power")] = pin.eth_power;
    ethObj[F("mdc")] = pin.eth_mdc;
    ethObj[F("mdio")] = pin.eth_mdio;
    ethObj[F("type")] = pin.eth_type;
    ethObj[F("clk_mode")] = pin.eth_clk_mode;

    response->setLength();
    request->send(response);
}

void WebApiDeviceClass::onDeviceAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, MQTT_JSON_DOC_SIZE);
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

    if (json.length() > MQTT_JSON_DOC_SIZE) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(MQTT_JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("curPin"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("curPin")][F("name")].as<String>().length() == 0 || root[F("curPin")][F("name")].as<String>().length() > DEV_MAX_MAPPING_NAME_STRLEN) {
        retMsg[F("message")] = F("Pin mapping must between 1 and " STR(DEV_MAX_MAPPING_NAME_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::HardwarePinMappingLength;
        retMsg[F("param")][F("max")] = DEV_MAX_MAPPING_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strlcpy(config.Dev_PinMapping, root[F("curPin")][F("name")].as<String>().c_str(), sizeof(config.Dev_PinMapping));
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    yield();
    delay(1000);
    yield();
    ESP.restart();
}
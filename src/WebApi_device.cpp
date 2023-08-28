// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_device.h"
#include "Configuration.h"
#include "Display_Graphic.h"
#include "PinMapping.h"
#include "Utils.h"
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
    curPin["name"] = config.Dev_PinMapping;

    JsonObject nrfPinObj = curPin.createNestedObject("nrf24");
    nrfPinObj["clk"] = pin.nrf24_clk;
    nrfPinObj["cs"] = pin.nrf24_cs;
    nrfPinObj["en"] = pin.nrf24_en;
    nrfPinObj["irq"] = pin.nrf24_irq;
    nrfPinObj["miso"] = pin.nrf24_miso;
    nrfPinObj["mosi"] = pin.nrf24_mosi;

    JsonObject cmtPinObj = curPin.createNestedObject("cmt");
    cmtPinObj["clk"] = pin.cmt_clk;
    cmtPinObj["cs"] = pin.cmt_cs;
    cmtPinObj["fcs"] = pin.cmt_fcs;
    cmtPinObj["sdio"] = pin.cmt_sdio;
    cmtPinObj["gpio2"] = pin.cmt_gpio2;
    cmtPinObj["gpio3"] = pin.cmt_gpio3;

    JsonObject ethPinObj = curPin.createNestedObject("eth");
    ethPinObj["enabled"] = pin.eth_enabled;
    ethPinObj["phy_addr"] = pin.eth_phy_addr;
    ethPinObj["power"] = pin.eth_power;
    ethPinObj["mdc"] = pin.eth_mdc;
    ethPinObj["mdio"] = pin.eth_mdio;
    ethPinObj["type"] = pin.eth_type;
    ethPinObj["clk_mode"] = pin.eth_clk_mode;

    JsonObject displayPinObj = curPin.createNestedObject("display");
    displayPinObj["type"] = pin.display_type;
    displayPinObj["data"] = pin.display_data;
    displayPinObj["clk"] = pin.display_clk;
    displayPinObj["cs"] = pin.display_cs;
    displayPinObj["reset"] = pin.display_reset;

    JsonObject ledPinObj = curPin.createNestedObject("led");
    ledPinObj["led0"] = pin.led[0];
    ledPinObj["led1"] = pin.led[1];

    JsonObject display = root.createNestedObject("display");
    display["rotation"] = config.Display_Rotation;
    display["power_safe"] = config.Display_PowerSafe;
    display["screensaver"] = config.Display_ScreenSaver;
    display["contrast"] = config.Display_Contrast;
    display["language"] = config.Display_Language;

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
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > MQTT_JSON_DOC_SIZE) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(MQTT_JSON_DOC_SIZE);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("curPin")
            || root.containsKey("display"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["curPin"]["name"].as<String>().length() == 0 || root["curPin"]["name"].as<String>().length() > DEV_MAX_MAPPING_NAME_STRLEN) {
        retMsg["message"] = "Pin mapping must between 1 and " STR(DEV_MAX_MAPPING_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::HardwarePinMappingLength;
        retMsg["param"]["max"] = DEV_MAX_MAPPING_NAME_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    bool performRestart = root["curPin"]["name"].as<String>() != config.Dev_PinMapping;

    strlcpy(config.Dev_PinMapping, root["curPin"]["name"].as<String>().c_str(), sizeof(config.Dev_PinMapping));
    config.Display_Rotation = root["display"]["rotation"].as<uint8_t>();
    config.Display_PowerSafe = root["display"]["power_safe"].as<bool>();
    config.Display_ScreenSaver = root["display"]["screensaver"].as<bool>();
    config.Display_Contrast = root["display"]["contrast"].as<uint8_t>();
    config.Display_Language = root["display"]["language"].as<uint8_t>();

    Display.setOrientation(config.Display_Rotation);
    Display.enablePowerSafe = config.Display_PowerSafe;
    Display.enableScreensaver = config.Display_ScreenSaver;
    Display.setContrast(config.Display_Contrast);
    Display.setLanguage(config.Display_Language);

    Configuration.write();

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    if (performRestart) {
        Utils::restartDtu();
    }
}
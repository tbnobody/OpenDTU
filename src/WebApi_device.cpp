// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_device.h"
#include "Configuration.h"
#include "Display_Graphic.h"
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

    JsonObject nrfPinObj = curPin.createNestedObject("nrf24");
    nrfPinObj[F("clk")] = pin.nrf24_clk;
    nrfPinObj[F("cs")] = pin.nrf24_cs;
    nrfPinObj[F("en")] = pin.nrf24_en;
    nrfPinObj[F("irq")] = pin.nrf24_irq;
    nrfPinObj[F("miso")] = pin.nrf24_miso;
    nrfPinObj[F("mosi")] = pin.nrf24_mosi;

    JsonObject ethPinObj = curPin.createNestedObject("eth");
    ethPinObj[F("enabled")] = pin.eth_enabled;
    ethPinObj[F("phy_addr")] = pin.eth_phy_addr;
    ethPinObj[F("power")] = pin.eth_power;
    ethPinObj[F("mdc")] = pin.eth_mdc;
    ethPinObj[F("mdio")] = pin.eth_mdio;
    ethPinObj[F("type")] = pin.eth_type;
    ethPinObj[F("clk_mode")] = pin.eth_clk_mode;

    JsonObject displayPinObj = curPin.createNestedObject("display");
    displayPinObj[F("type")] = pin.display_type;
    displayPinObj[F("data")] = pin.display_data;
    displayPinObj[F("clk")] = pin.display_clk;
    displayPinObj[F("cs")] = pin.display_cs;
    displayPinObj[F("reset")] = pin.display_reset;

    JsonObject display = root.createNestedObject("display");
    display[F("rotation")] = config.Display_Rotation;
    display[F("power_safe")] = config.Display_PowerSafe;
    display[F("screensaver")] = config.Display_ScreenSaver;
    display[F("contrast")] = config.Display_Contrast;

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

    if (!(root.containsKey("curPin") || root.containsKey("display"))) {
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
    bool performRestart = root[F("curPin")][F("name")].as<String>() != config.Dev_PinMapping;

    strlcpy(config.Dev_PinMapping, root[F("curPin")][F("name")].as<String>().c_str(), sizeof(config.Dev_PinMapping));
    config.Display_Rotation = root[F("display")][F("rotation")].as<uint8_t>();
    config.Display_PowerSafe = root[F("display")][F("power_safe")].as<bool>();
    config.Display_ScreenSaver = root[F("display")][F("screensaver")].as<bool>();
    config.Display_Contrast = root[F("display")][F("contrast")].as<uint8_t>();

    Display.setOrientation(config.Display_Rotation);
    Display.enablePowerSafe = config.Display_PowerSafe;
    Display.enableScreensaver = config.Display_ScreenSaver;
    Display.setContrast(config.Display_Contrast);

    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    if (performRestart) {
        yield();
        delay(1000);
        yield();
        ESP.restart();
    }
}
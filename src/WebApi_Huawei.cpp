// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_Huawei.h"
#include "Huawei_can.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiHuaweiClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/huawei/status", HTTP_GET, std::bind(&WebApiHuaweiClass::onStatus, this, _1));
    _server->on("/api/huawei/config", HTTP_GET, std::bind(&WebApiHuaweiClass::onAdminGet, this, _1));
    _server->on("/api/huawei/config", HTTP_POST, std::bind(&WebApiHuaweiClass::onAdminPost, this, _1));
    _server->on("/api/huawei/limit/config", HTTP_POST, std::bind(&WebApiHuaweiClass::onPost, this, _1));
}

void WebApiHuaweiClass::loop()
{
}

void WebApiHuaweiClass::getJsonData(JsonObject& root) {
    const RectifierParameters_t * rp = HuaweiCan.get();

    root["data_age"] = (millis() - HuaweiCan.getLastUpdate()) / 1000;
    root[F("input_voltage")]["v"] = rp->input_voltage;
    root[F("input_voltage")]["u"] = "V";
    root[F("input_current")]["v"] = rp->input_current;
    root[F("input_current")]["u"] = "A";
    root[F("input_power")]["v"] = rp->input_power;
    root[F("input_power")]["u"] = "W";
    root[F("output_voltage")]["v"] = rp->output_voltage;
    root[F("output_voltage")]["u"] = "V";
    root[F("output_current")]["v"] = rp->output_current;
    root[F("output_current")]["u"] = "A";
    root[F("max_output_current")]["v"] = rp->max_output_current;
    root[F("max_output_current")]["u"] = "A";
    root[F("output_power")]["v"] = rp->output_power;
    root[F("output_power")]["u"] = "W";
    root[F("input_temp")]["v"] = rp->input_temp;
    root[F("input_temp")]["u"] = "°C";
    root[F("output_temp")]["v"] = rp->output_temp;
    root[F("output_temp")]["u"] = "°C";
    root[F("efficiency")]["v"] = rp->efficiency * 100;
    root[F("efficiency")]["u"] = "%";

}

void WebApiHuaweiClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    getJsonData(root);

    response->setLength();
    request->send(response);
}

void WebApiHuaweiClass::onPost(AsyncWebServerRequest* request)
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
    float value;
    uint8_t online = true;
    float minimal_voltage;

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (root.containsKey("online")) {
        online = root[F("online")].as<bool>();
        if (online) {
            minimal_voltage = HUAWEI_MINIMAL_ONLINE_VOLTAGE;
        } else {
            minimal_voltage = HUAWEI_MINIMAL_OFFLINE_VOLTAGE;
        }
    } else {
        retMsg[F("message")] = F("Could not read info if data should be set for online/offline operation!");
        retMsg[F("code")] = WebApiError::LimitInvalidType;
        response->setLength();
        request->send(response);
        return;
    }

    if (root.containsKey("voltage_valid")) {
        if (root[F("voltage_valid")].as<bool>()) {
            if (root[F("voltage")].as<float>() < minimal_voltage || root[F("voltage")].as<float>() > 58) {
                retMsg[F("message")] = F("voltage not in range between 42 (online)/48 (offline and 58V !");
                retMsg[F("code")] = WebApiError::LimitInvalidLimit;
                retMsg[F("param")][F("max")] = 58;
                retMsg[F("param")][F("min")] = minimal_voltage;
                response->setLength();
                request->send(response);
                return;
            } else {
                value = root[F("voltage")].as<float>();
                if (online) {
                    HuaweiCan.setValue(value, HUAWEI_ONLINE_VOLTAGE);
                } else {
                    HuaweiCan.setValue(value, HUAWEI_OFFLINE_VOLTAGE);
                }
            }
        }
    }

    if (root.containsKey("current_valid")) {
        if (root[F("current_valid")].as<bool>()) {
            if (root[F("current")].as<float>() < 0 || root[F("current")].as<float>() > 60) {
                retMsg[F("message")] = F("current must be in range between 0 and 60!");
                retMsg[F("code")] = WebApiError::LimitInvalidLimit;
                retMsg[F("param")][F("max")] = 60;
                retMsg[F("param")][F("min")] = 0;
                response->setLength();
                request->send(response);
                return;
            } else {
                value = root[F("current")].as<float>();
                if (online) {
                    HuaweiCan.setValue(value, HUAWEI_ONLINE_CURRENT);
                } else {
                    HuaweiCan.setValue(value, HUAWEI_OFFLINE_CURRENT);
                }
            }
        }
    }

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}




void WebApiHuaweiClass::onAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("enabled")] = config.Huawei_Enabled;
    root[F("can_controller_frequency")] = config.Huawei_CAN_Controller_Frequency;
    root[F("auto_power_enabled")] = config.Huawei_Auto_Power_Enabled;
    root[F("voltage_limit")] = static_cast<int>(config.Huawei_Auto_Power_Voltage_Limit * 100) / 100.0;
    root[F("enable_voltage_limit")] = static_cast<int>(config.Huawei_Auto_Power_Enable_Voltage_Limit * 100) / 100.0;
    root[F("lower_power_limit")] = config.Huawei_Auto_Power_Lower_Power_Limit;
    root[F("upper_power_limit")] = config.Huawei_Auto_Power_Upper_Power_Limit;   

    response->setLength();
    request->send(response);
}

void WebApiHuaweiClass::onAdminPost(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("enabled")) ||
        !(root.containsKey("can_controller_frequency")) ||
        !(root.containsKey("auto_power_enabled")) ||
        !(root.containsKey("voltage_limit")) ||
        !(root.containsKey("lower_power_limit")) ||
        !(root.containsKey("upper_power_limit"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.Huawei_Enabled = root[F("enabled")].as<bool>();
    config.Huawei_CAN_Controller_Frequency = root[F("can_controller_frequency")].as<uint32_t>();
    config.Huawei_Auto_Power_Enabled = root[F("auto_power_enabled")].as<bool>();
    config.Huawei_Auto_Power_Voltage_Limit = root[F("voltage_limit")].as<float>();
    config.Huawei_Auto_Power_Enable_Voltage_Limit = root[F("enable_voltage_limit")].as<float>();
    config.Huawei_Auto_Power_Lower_Power_Limit = root[F("lower_power_limit")].as<float>();
    config.Huawei_Auto_Power_Upper_Power_Limit = root[F("upper_power_limit")].as<float>();    
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    // TODO(schlimmchen): HuaweiCan has no real concept of the fact that the
    // config might change. at least not regarding CAN parameters. until that
    // changes, the ESP must restart for configuration changes to take effect.
    yield();
    delay(1000);
    yield();
    ESP.restart();

    const PinMapping_t& pin = PinMapping.get();
    // Properly turn this on
    if (config.Huawei_Enabled) {
        MessageOutput.println(F("Initialize Huawei AC charger interface... "));
        if (PinMapping.isValidHuaweiConfig()) {
            MessageOutput.printf("Huawei AC-charger miso = %d, mosi = %d, clk = %d, irq = %d, cs = %d, power_pin = %d\r\n", pin.huawei_miso, pin.huawei_mosi, pin.huawei_clk, pin.huawei_irq, pin.huawei_cs, pin.huawei_power);
            HuaweiCan.init(pin.huawei_miso, pin.huawei_mosi, pin.huawei_clk, pin.huawei_irq, pin.huawei_cs, pin.huawei_power);
            MessageOutput.println(F("done"));
        } else {
            MessageOutput.println(F("Invalid pin config"));
        }
    }

    // Properly turn this off
    if (!config.Huawei_Enabled) {
      HuaweiCan.setValue(0, HUAWEI_ONLINE_CURRENT);
      delay(500);
      HuaweiCan.setMode(HUAWEI_MODE_OFF);
      return;
    }

    if (config.Huawei_Auto_Power_Enabled) {
      HuaweiCan.setMode(HUAWEI_MODE_AUTO_INT);
      return;
    }

    HuaweiCan.setMode(HUAWEI_MODE_AUTO_EXT);
}

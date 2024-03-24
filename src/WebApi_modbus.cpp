// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Bobby Noelte
 */
#include "WebApi_modbus.h"
#include "NetworkSettings.h"
#include "ModbusSettings.h"
#include "Configuration.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiModbusClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/modbus/status", HTTP_GET, std::bind(&WebApiModbusClass::onModbusStatus, this, _1));
    server.on("/api/modbus/config", HTTP_GET, std::bind(&WebApiModbusClass::onModbusAdminGet, this, _1));
    server.on("/api/modbus/config", HTTP_POST, std::bind(&WebApiModbusClass::onModbusAdminPost, this, _1));
}

void WebApiModbusClass::onModbusStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["modbus_tcp_enabled"] = config.Modbus.TCPEnabled;
    root["modbus_hostname"] = NetworkSettings.getHostname();
    root["modbus_ip"] = NetworkSettings.localIP().toString();
    root["modbus_port"] = config.Modbus.Port;
    root["modbus_clients"] = config.Modbus.Clients;
    root["modbus_id_dtupro"] = config.Modbus.IDDTUPro;
    root["modbus_id_total"] = config.Modbus.IDTotal;
    root["modbus_id_meter"] = config.Modbus.IDMeter;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiModbusClass::onModbusAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["modbus_tcp_enabled"] = config.Modbus.TCPEnabled;
    root["modbus_clients"] = config.Modbus.Clients;
    root["modbus_id_dtupro"] = config.Modbus.IDDTUPro;
    root["modbus_id_total"] = config.Modbus.IDTotal;
    root["modbus_id_meter"] = config.Modbus.IDMeter;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiModbusClass::onModbusAdminPost(AsyncWebServerRequest* request)
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

    if (!root.containsKey("modbus_tcp_enabled") ||
        !root.containsKey("modbus_clients") ||
        !root.containsKey("modbus_id_dtupro") ||
        !root.containsKey("modbus_id_total") ||
        !root.containsKey("modbus_id_meter")) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    CONFIG_T& config = Configuration.get();

    if (root["modbus_tcp_enabled"].as<bool>()) {
        config.Modbus.Clients = root["modbus_clients"].as<uint32_t>();
        config.Modbus.IDDTUPro = root["modbus_id_dtupro"].as<uint32_t>();
        config.Modbus.IDTotal = root["modbus_id_total"].as<uint32_t>();
        config.Modbus.IDMeter = root["modbus_id_meter"].as<uint32_t>();
    }
    config.Modbus.TCPEnabled = root["modbus_tcp_enabled"].as<bool>();

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    ModbusSettings.performConfig();
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_wireguard.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>

void WebApiWireguardClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/wireguard/status", HTTP_GET, std::bind(&WebApiWireguardClass::onWireguardStatus, this, _1));
    _server->on("/api/wireguard/config", HTTP_GET, std::bind(&WebApiWireguardClass::onWireguardAdminGet, this, _1));
    _server->on("/api/wireguard/config", HTTP_POST, std::bind(&WebApiWireguardClass::onWireguardAdminPost, this, _1));
}

void WebApiWireguardClass::loop()
{
}

void WebApiWireguardClass::onWireguardStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    const CONFIG_T& config = Configuration.get();
    root[F("wg_enabled")] = config.Wg_Enabled;

    root[F("wg_endpoint_address")] = config.Wg_Endpoint_Address;
    root[F("wg_endpoint_local_ip")] = IPAddress(config.Wg_Endpoint_Local_Ip).toString();
    root[F("wg_endpoint_port")] = config.Wg_Endpoint_Port;
    root[F("wg_endpoint_public_key")] = config.Wg_Endpoint_Public_Key;

    root[F("wg_opendtu_local_ip")] = IPAddress(config.Wg_Opendtu_Local_Ip).toString();
    root[F("wg_opendtu_public_key")] = config.Wg_Opendtu_Public_Key;

    response->setLength();
    request->send(response);
}

void WebApiWireguardClass::onWireguardAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("wg_enabled")] = config.Wg_Enabled;

    root[F("wg_endpoint_address")] = config.Wg_Endpoint_Address;
    root[F("wg_endpoint_local_ip")] = IPAddress(config.Wg_Endpoint_Local_Ip).toString();
    root[F("wg_endpoint_port")] = config.Wg_Endpoint_Port;
    root[F("wg_endpoint_public_key")] = config.Wg_Endpoint_Public_Key;

    root[F("wg_opendtu_local_ip")] = IPAddress(config.Wg_Opendtu_Local_Ip).toString();
    root[F("wg_opendtu_public_key")] = config.Wg_Opendtu_Public_Key;
    root[F("wg_opendtu_private_key")] = config.Wg_Opendtu_Private_Key;

    response->setLength();
    request->send(response);
}

void WebApiWireguardClass::onWireguardAdminPost(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("wg_enabled") && root.containsKey("wg_endpoint_address") 
        && root.containsKey("wg_endpoint_local_ip") && root.containsKey("wg_endpoint_port") 
        && root.containsKey("wg_endpoint_public_key") && root.containsKey("wg_opendtu_local_ip") 
        && root.containsKey("wg_opendtu_public_key") && root.containsKey("wg_opendtu_private_key"))) {

        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }
    CONFIG_T& config = Configuration.get();
    config.Wg_Refresh = true;
    IPAddress Wg_Endpoint_Local_Ip;
    if (Wg_Endpoint_Local_Ip.fromString(root[F("wg_endpoint_local_ip")].as<String>())) {
        config.Wg_Endpoint_Local_Ip[0] = Wg_Endpoint_Local_Ip[0];
        config.Wg_Endpoint_Local_Ip[1] = Wg_Endpoint_Local_Ip[1];
        config.Wg_Endpoint_Local_Ip[2] = Wg_Endpoint_Local_Ip[2];
        config.Wg_Endpoint_Local_Ip[3] = Wg_Endpoint_Local_Ip[3];
    }
    IPAddress Wg_Opendtu_Local_Ip;
    if (Wg_Opendtu_Local_Ip.fromString(root[F("wg_opendtu_local_ip")].as<String>())) {
        config.Wg_Opendtu_Local_Ip[0] = Wg_Opendtu_Local_Ip[0];
        config.Wg_Opendtu_Local_Ip[1] = Wg_Opendtu_Local_Ip[1];
        config.Wg_Opendtu_Local_Ip[2] = Wg_Opendtu_Local_Ip[2];
        config.Wg_Opendtu_Local_Ip[3] = Wg_Opendtu_Local_Ip[3];
    }
    config.Wg_Enabled = root[F("wg_enabled")].as<bool>();
    strlcpy(config.Wg_Opendtu_Public_Key, root[F("wg_opendtu_public_key")].as<String>().c_str(), sizeof(config.Wg_Opendtu_Public_Key));
    strlcpy(config.Wg_Opendtu_Private_Key, root[F("wg_opendtu_private_key")].as<String>().c_str(), sizeof(config.Wg_Opendtu_Private_Key));
    strlcpy(config.Wg_Endpoint_Public_Key, root[F("wg_endpoint_public_key")].as<String>().c_str(), sizeof(config.Wg_Endpoint_Public_Key));
    strlcpy(config.Wg_Endpoint_Address, root[F("wg_endpoint_address")].as<String>().c_str(), sizeof(config.Wg_Endpoint_Address));
    config.Wg_Endpoint_Port = root[F("wg_endpoint_port")].as<uint>();
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}
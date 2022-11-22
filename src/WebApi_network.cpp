// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_network.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include "WebApi.h"
#include "helper.h"

void WebApiNetworkClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/network/status", HTTP_GET, std::bind(&WebApiNetworkClass::onNetworkStatus, this, _1));
    _server->on("/api/network/config", HTTP_GET, std::bind(&WebApiNetworkClass::onNetworkAdminGet, this, _1));
    _server->on("/api/network/config", HTTP_POST, std::bind(&WebApiNetworkClass::onNetworkAdminPost, this, _1));
}

void WebApiNetworkClass::loop()
{
}

void WebApiNetworkClass::onNetworkStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root[F("sta_status")] = ((WiFi.getMode() & WIFI_STA) != 0);
    root[F("sta_ssid")] = WiFi.SSID();
    root[F("sta_rssi")] = WiFi.RSSI();
    root[F("network_hostname")] = NetworkSettings.getHostname();
    root[F("network_ip")] = NetworkSettings.localIP().toString();
    root[F("network_netmask")] = NetworkSettings.subnetMask().toString();
    root[F("network_gateway")] = NetworkSettings.gatewayIP().toString();
    root[F("network_dns1")] = NetworkSettings.dnsIP(0).toString();
    root[F("network_dns2")] = NetworkSettings.dnsIP(1).toString();
    root[F("network_mac")] = NetworkSettings.macAddress();
    root[F("network_mode")] = NetworkSettings.NetworkMode() == network_mode::WiFi ? F("Station") : F("Ethernet");
    root[F("ap_status")] = ((WiFi.getMode() & WIFI_AP) != 0);
    root[F("ap_ssid")] = NetworkSettings.getApName();
    root[F("ap_ip")] = WiFi.softAPIP().toString();
    root[F("ap_mac")] = WiFi.softAPmacAddress();
    root[F("ap_stationnum")] = WiFi.softAPgetStationNum();

    response->setLength();
    request->send(response);
}

void WebApiNetworkClass::onNetworkAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("hostname")] = config.WiFi_Hostname;
    root[F("dhcp")] = config.WiFi_Dhcp;
    root[F("ipaddress")] = IPAddress(config.WiFi_Ip).toString();
    root[F("netmask")] = IPAddress(config.WiFi_Netmask).toString();
    root[F("gateway")] = IPAddress(config.WiFi_Gateway).toString();
    root[F("dns1")] = IPAddress(config.WiFi_Dns1).toString();
    root[F("dns2")] = IPAddress(config.WiFi_Dns2).toString();
    root[F("ssid")] = config.WiFi_Ssid;
    root[F("password")] = config.WiFi_Password;

    response->setLength();
    request->send(response);
}

void WebApiNetworkClass::onNetworkAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("ssid") && root.containsKey("password") && root.containsKey("hostname") && root.containsKey("dhcp") && root.containsKey("ipaddress") && root.containsKey("netmask") && root.containsKey("gateway") && root.containsKey("dns1") && root.containsKey("dns2"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    IPAddress ipaddress;
    if (!ipaddress.fromString(root[F("ipaddress")].as<String>())) {
        retMsg[F("message")] = F("IP address is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress netmask;
    if (!netmask.fromString(root[F("netmask")].as<String>())) {
        retMsg[F("message")] = F("Netmask is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress gateway;
    if (!gateway.fromString(root[F("gateway")].as<String>())) {
        retMsg[F("message")] = F("Gateway is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress dns1;
    if (!dns1.fromString(root[F("dns1")].as<String>())) {
        retMsg[F("message")] = F("DNS Server IP 1 is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress dns2;
    if (!dns2.fromString(root[F("dns2")].as<String>())) {
        retMsg[F("message")] = F("DNS Server IP 2 is invalid!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("hostname")].as<String>().length() == 0 || root[F("hostname")].as<String>().length() > WIFI_MAX_HOSTNAME_STRLEN) {
        retMsg[F("message")] = F("Hostname must between 1 and " STR(WIFI_MAX_HOSTNAME_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }
    if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
        if (root[F("ssid")].as<String>().length() == 0 || root[F("ssid")].as<String>().length() > WIFI_MAX_SSID_STRLEN) {
            retMsg[F("message")] = F("SSID must between 1 and " STR(WIFI_MAX_SSID_STRLEN) " characters long!");
            response->setLength();
            request->send(response);
            return;
        }
    }
    if (root[F("password")].as<String>().length() > WIFI_MAX_PASSWORD_STRLEN - 1) {
        retMsg[F("message")] = F("Password must not be longer than " STR(WIFI_MAX_PASSWORD_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.WiFi_Ip[0] = ipaddress[0];
    config.WiFi_Ip[1] = ipaddress[1];
    config.WiFi_Ip[2] = ipaddress[2];
    config.WiFi_Ip[3] = ipaddress[3];
    config.WiFi_Netmask[0] = netmask[0];
    config.WiFi_Netmask[1] = netmask[1];
    config.WiFi_Netmask[2] = netmask[2];
    config.WiFi_Netmask[3] = netmask[3];
    config.WiFi_Gateway[0] = gateway[0];
    config.WiFi_Gateway[1] = gateway[1];
    config.WiFi_Gateway[2] = gateway[2];
    config.WiFi_Gateway[3] = gateway[3];
    config.WiFi_Dns1[0] = dns1[0];
    config.WiFi_Dns1[1] = dns1[1];
    config.WiFi_Dns1[2] = dns1[2];
    config.WiFi_Dns1[3] = dns1[3];
    config.WiFi_Dns2[0] = dns2[0];
    config.WiFi_Dns2[1] = dns2[1];
    config.WiFi_Dns2[2] = dns2[2];
    config.WiFi_Dns2[3] = dns2[3];
    strlcpy(config.WiFi_Ssid, root[F("ssid")].as<String>().c_str(), sizeof(config.WiFi_Ssid));
    strlcpy(config.WiFi_Password, root[F("password")].as<String>().c_str(), sizeof(config.WiFi_Password));
    strlcpy(config.WiFi_Hostname, root[F("hostname")].as<String>().c_str(), sizeof(config.WiFi_Hostname));
    if (root[F("dhcp")].as<bool>()) {
        config.WiFi_Dhcp = true;
    } else {
        config.WiFi_Dhcp = false;
    }
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    NetworkSettings.enableAdminMode();
    NetworkSettings.applyConfig();
}
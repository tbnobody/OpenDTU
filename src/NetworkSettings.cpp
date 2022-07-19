// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "NetworkSettings.h"
#include "Configuration.h"
#include "defaults.h"
#include <WiFi.h>
#ifdef OPENDTU_ETHERNET
#include <ETH.h>
#endif

NetworkSettingsClass::NetworkSettingsClass()
    : apIp(192, 168, 4, 1)
    , apNetmask(255, 255, 255, 0)
{
    dnsServer.reset(new DNSServer());
}

void NetworkSettingsClass::init()
{
    using namespace std::placeholders;

    WiFi.onEvent(std::bind(&NetworkSettingsClass::NetworkEvent, this, _1));
    setupMode();
}

void NetworkSettingsClass::NetworkEvent(WiFiEvent_t event)
{
    switch (event) {
#ifdef OPENDTU_ETHERNET
    case ARDUINO_EVENT_ETH_START:
        Serial.println(F("ETH start"));
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_START);
        }
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println(F("ETH stop"));
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_STOP);
        }
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println(F("ETH connected"));
        _ethConnected = true;
        raiseEvent(network_event::NETWORK_CONNECTED);
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.printf("ETH got IP: %s\n", ETH.localIP().toString().c_str());
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println(F("ETH disconnected"));
        _ethConnected = false;
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
#endif
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println(F("WiFi connected"));
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_CONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println(F("WiFi disconnected"));
        if (_networkMode == network_mode::WiFi) {
            Serial.println(F("Try reconnecting"));
            WiFi.reconnect();
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("WiFi got ip: %s\n", WiFi.localIP().toString().c_str());
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    default:
        break;
    }
}

bool NetworkSettingsClass::onEvent(NetworkEventCb cbEvent, network_event event)
{
    if (!cbEvent) {
        return pdFALSE;
    }
    NetworkEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.event = event;
    _cbEventList.push_back(newEventHandler);
    return true;
}

void NetworkSettingsClass::raiseEvent(network_event event)
{
    for (uint32_t i = 0; i < _cbEventList.size(); i++) {
        NetworkEventCbList_t entry = _cbEventList[i];
        if (entry.cb) {
            if (entry.event == event || entry.event == network_event::NETWORK_EVENT_MAX) {
                entry.cb(event);
            }
        }
    }
}

void NetworkSettingsClass::setupMode()
{
    if (adminEnabled) {
        WiFi.mode(WIFI_AP_STA);
        String ssidString = getApName();
        WiFi.softAPConfig(apIp, apIp, apNetmask);
        WiFi.softAP((const char*)ssidString.c_str(), ACCESS_POINT_PASSWORD);
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
        dnsServerStatus = true;
    } else {
        dnsServer->stop();
        dnsServerStatus = false;
        if (_networkMode == network_mode::WiFi) {
            WiFi.mode(WIFI_STA);
        } else {
            WiFi.mode(WIFI_MODE_NULL);
        }
    }
#ifdef OPENDTU_ETHERNET
    ETH.begin();
#endif
}

void NetworkSettingsClass::enableAdminMode()
{
    adminEnabled = true;
    adminTimeoutCounter = 0;
    setupMode();
}

String NetworkSettingsClass::getApName()
{
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i += 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return String(ACCESS_POINT_NAME + String(chipId));
}

void NetworkSettingsClass::loop()
{
#ifdef OPENDTU_ETHERNET
    if (_ethConnected) {
        if (_networkMode != network_mode::Ethernet) {
            // Do stuff when switching to Ethernet mode
            Serial.println(F("Switch to Ethernet mode"));
            _networkMode = network_mode::Ethernet;
            WiFi.mode(WIFI_MODE_NULL);
            setStaticIp();
            setHostname();
        }
    } else
#endif
        if (_networkMode != network_mode::WiFi) {
        // Do stuff when switching to Ethernet mode
        Serial.println(F("Switch to WiFi mode"));
        _networkMode = network_mode::WiFi;
        enableAdminMode();
        applyConfig();
    }

    if (millis() - lastTimerCall > 1000) {
        adminTimeoutCounter++;
        connectTimeoutTimer++;
        connectRedoTimer++;
        lastTimerCall = millis();
    }
    if (adminEnabled) {
        // Don't disable the admin mode when network is not available
        if (!isConnected()) {
            adminTimeoutCounter = 0;
        }
        // If WiFi is connected to AP for more than ADMIN_TIMEOUT
        // seconds, disable the internal Access Point
        if (adminTimeoutCounter > ADMIN_TIMEOUT) {
            adminEnabled = false;
            Serial.println(F("Admin mode disabled"));
            setupMode();
        }
        // It's nearly not possible to use the internal AP if the
        // WiFi is searching for an AP. So disable searching afer
        // WIFI_RECONNECT_TIMEOUT and repeat after WIFI_RECONNECT_REDO_TIMEOUT
        if (isConnected()) {
            connectTimeoutTimer = 0;
            connectRedoTimer = 0;
        } else {
            if (connectTimeoutTimer > WIFI_RECONNECT_TIMEOUT && !forceDisconnection) {
                Serial.print(F("Disable search for AP... "));
                WiFi.mode(WIFI_AP);
                Serial.println(F("done"));
                connectRedoTimer = 0;
                forceDisconnection = true;
            }
            if (connectRedoTimer > WIFI_RECONNECT_REDO_TIMEOUT && forceDisconnection) {
                Serial.print(F("Enable search for AP... "));
                WiFi.mode(WIFI_AP_STA);
                Serial.println(F("done"));
                applyConfig();
                connectTimeoutTimer = 0;
                forceDisconnection = false;
            }
        }
    }
    if (dnsServerStatus) {
        dnsServer->processNextRequest();
    }
}

void NetworkSettingsClass::applyConfig()
{
    setHostname();
    if (!strcmp(Configuration.get().WiFi_Ssid, "")) {
        return;
    }
    Serial.print(F("Configuring WiFi STA using "));
    if (strcmp(WiFi.SSID().c_str(), Configuration.get().WiFi_Ssid) || strcmp(WiFi.psk().c_str(), Configuration.get().WiFi_Password)) {
        Serial.print(F("new credentials... "));
        WiFi.begin(
            Configuration.get().WiFi_Ssid,
            Configuration.get().WiFi_Password);
    } else {
        Serial.print(F("existing credentials... "));
        WiFi.begin();
    }
    Serial.println(F("done"));
    setStaticIp();
}

void NetworkSettingsClass::setHostname()
{
    Serial.print(F("Setting Hostname... "));
    if (strcmp(Configuration.get().WiFi_Hostname, "")) {
        if (_networkMode == network_mode::WiFi) {
            if (WiFi.hostname(Configuration.get().WiFi_Hostname)) {
                Serial.println(F("done"));
            } else {
                Serial.println(F("failed"));
            }
        }
#ifdef OPENDTU_ETHERNET
        else if (_networkMode == network_mode::Ethernet) {
            if (ETH.setHostname(Configuration.get().WiFi_Hostname)) {
                Serial.println(F("done"));
            } else {
                Serial.println(F("failed"));
            }
        }
#endif
    } else {
        Serial.println(F("failed (Hostname empty)"));
    }
}

void NetworkSettingsClass::setStaticIp()
{
    if (_networkMode == network_mode::WiFi) {
        if (!Configuration.get().WiFi_Dhcp) {
            Serial.print(F("Configuring WiFi STA static IP... "));
            WiFi.config(
                IPAddress(Configuration.get().WiFi_Ip),
                IPAddress(Configuration.get().WiFi_Gateway),
                IPAddress(Configuration.get().WiFi_Netmask),
                IPAddress(Configuration.get().WiFi_Dns1),
                IPAddress(Configuration.get().WiFi_Dns2));
            Serial.println(F("done"));
        }
    }
#ifdef OPENDTU_ETHERNET
    else if (_networkMode == network_mode::Ethernet) {
        if (Configuration.get().WiFi_Dhcp) {
            ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        } else {
            Serial.print(F("Configuring Ethernet static IP... "));
            ETH.config(
                IPAddress(Configuration.get().WiFi_Ip),
                IPAddress(Configuration.get().WiFi_Gateway),
                IPAddress(Configuration.get().WiFi_Netmask),
                IPAddress(Configuration.get().WiFi_Dns1),
                IPAddress(Configuration.get().WiFi_Dns2));
            Serial.println(F("done"));
        }
    }
#endif
}

IPAddress NetworkSettingsClass::localIP()
{
    switch (_networkMode) {
#ifdef OPENDTU_ETHERNET
    case network_mode::Ethernet:
        return ETH.localIP();
        break;
#endif
    case network_mode::WiFi:
        return WiFi.localIP();
        break;
    default:
        return INADDR_NONE;
    }
}

IPAddress NetworkSettingsClass::subnetMask()
{
    switch (_networkMode) {
#ifdef OPENDTU_ETHERNET
    case network_mode::Ethernet:
        return ETH.subnetMask();
        break;
#endif
    case network_mode::WiFi:
        return WiFi.subnetMask();
        break;
    default:
        return IPAddress(255, 255, 255, 0);
    }
}

IPAddress NetworkSettingsClass::gatewayIP()
{
    switch (_networkMode) {
#ifdef OPENDTU_ETHERNET
    case network_mode::Ethernet:
        return ETH.gatewayIP();
        break;
#endif
    case network_mode::WiFi:
        return WiFi.gatewayIP();
        break;
    default:
        return INADDR_NONE;
    }
}

IPAddress NetworkSettingsClass::dnsIP(uint8_t dns_no)
{
    switch (_networkMode) {
#ifdef OPENDTU_ETHERNET
    case network_mode::Ethernet:
        return ETH.dnsIP(dns_no);
        break;
#endif
    case network_mode::WiFi:
        return WiFi.dnsIP(dns_no);
        break;
    default:
        return INADDR_NONE;
    }
}

String NetworkSettingsClass::macAddress()
{
    switch (_networkMode) {
#ifdef OPENDTU_ETHERNET
    case network_mode::Ethernet:
        return ETH.macAddress();
        break;
#endif
    case network_mode::WiFi:
        return WiFi.macAddress();
        break;
    default:
        return String("");
    }
}

const char* NetworkSettingsClass::getHostname()
{
#ifdef OPENDTU_ETHERNET
    if (_networkMode == network_mode::Ethernet) {
        return ETH.getHostname();
    }
#endif
    return WiFi.getHostname();
}

bool NetworkSettingsClass::isConnected()
{
#ifndef OPENDTU_ETHERNET
    return WiFi.localIP()[0] != 0;
#else
    return WiFi.localIP()[0] != 0 || ETH.localIP()[0] != 0;
#endif
}

network_mode NetworkSettingsClass::NetworkMode()
{
    return _networkMode;
}

NetworkSettingsClass NetworkSettings;
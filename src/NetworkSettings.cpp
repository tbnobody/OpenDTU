// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "NetworkSettings.h"
#include "Configuration.h"
#include "Utils.h"
#include "defaults.h"
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
    using std::placeholders::_1;

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
        WiFi.softAP((const char*)ssidString.c_str(), Configuration.get().Security_Password);
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
    return String(ACCESS_POINT_NAME + String(Utils::getChipId()));
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
    if (_networkMode == network_mode::WiFi) {
        if (WiFi.hostname(getHostname())) {
            Serial.println(F("done"));
        } else {
            Serial.println(F("failed"));
        }

        // Evil bad hack to get the hostname set up correctly
        WiFi.mode(WIFI_MODE_APSTA);
        WiFi.mode(WIFI_MODE_STA);
        setupMode();
    }
#ifdef OPENDTU_ETHERNET
    else if (_networkMode == network_mode::Ethernet) {
        if (ETH.setHostname(getHostname().c_str())) {
            Serial.println(F("done"));
        } else {
            Serial.println(F("failed"));
        }
    }
#endif
}

void NetworkSettingsClass::setStaticIp()
{
    if (_networkMode == network_mode::WiFi) {
        if (Configuration.get().WiFi_Dhcp) {
            Serial.print(F("Configuring WiFi STA DHCP IP... "));
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            Serial.println(F("done"));
        } else {
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
            Serial.print(F("Configuring Ethernet DHCP IP... "));
            ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
            Serial.println(F("done"));
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
        return "";
    }
}

String NetworkSettingsClass::getHostname()
{
    const CONFIG_T& config = Configuration.get();
    char preparedHostname[WIFI_MAX_HOSTNAME_STRLEN + 1];
    char resultHostname[WIFI_MAX_HOSTNAME_STRLEN + 1];
    uint8_t pos = 0;

    uint32_t chipId = Utils::getChipId();
    snprintf(preparedHostname, WIFI_MAX_HOSTNAME_STRLEN + 1, config.WiFi_Hostname, chipId);

    const char* pC = preparedHostname;
    while (*pC && pos < WIFI_MAX_HOSTNAME_STRLEN) { // while !null and not over length
        if (isalnum(*pC)) { // if the current char is alpha-numeric append it to the hostname
            resultHostname[pos] = *pC;
            pos++;
        } else if (*pC == ' ' || *pC == '_' || *pC == '-' || *pC == '+' || *pC == '!' || *pC == '?' || *pC == '*') {
            resultHostname[pos] = '-';
            pos++;
        }
        // else do nothing - no leading hyphens and do not include hyphens for all other characters.
        pC++;
    }

    resultHostname[pos] = '\0'; // terminate string

    // last character must not be hyphen
    while (pos > 0 && resultHostname[pos - 1] == '-') {
        resultHostname[pos - 1] = '\0';
        pos--;
    }

    // Fallback if no other rule applied
    if (strlen(resultHostname) == 0) {
        snprintf(resultHostname, WIFI_MAX_HOSTNAME_STRLEN + 1, APP_HOSTNAME, chipId);
    }

    return resultHostname;
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
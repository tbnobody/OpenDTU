// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "NetworkSettings.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include "Utils.h"
#include "defaults.h"
#include <ESPmDNS.h>
#include <ETH.h>
#include "__compiled_constants.h"

NetworkSettingsClass::NetworkSettingsClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&NetworkSettingsClass::loop, this))
    , _apIp(192, 168, 4, 1)
    , _apNetmask(255, 255, 255, 0)
{
    _dnsServer.reset(new DNSServer());
}

void NetworkSettingsClass::init(Scheduler& scheduler)
{
    using std::placeholders::_1;

    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

    WiFi.disconnect(true, true);

    WiFi.onEvent(std::bind(&NetworkSettingsClass::NetworkEvent, this, _1));
    setupMode();

    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void NetworkSettingsClass::NetworkEvent(const WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_ETH_START:
        MessageOutput.println("ETH start");
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_START);
        }
        break;
    case ARDUINO_EVENT_ETH_STOP:
        MessageOutput.println("ETH stop");
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_STOP);
        }
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        MessageOutput.println("ETH connected");
        _ethConnected = true;
        raiseEvent(network_event::NETWORK_CONNECTED);
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        MessageOutput.printf("ETH got IP: %s\r\n", ETH.localIP().toString().c_str());
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        MessageOutput.println("ETH disconnected");
        _ethConnected = false;
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        MessageOutput.println("WiFi connected");
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_CONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        MessageOutput.println("WiFi disconnected");
        if (_networkMode == network_mode::WiFi) {
            MessageOutput.println("Try reconnecting");
            WiFi.disconnect(true, true);
            WiFi.reconnect();
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        MessageOutput.printf("WiFi got ip: %s\r\n", WiFi.localIP().toString().c_str());
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    default:
        break;
    }
}

bool NetworkSettingsClass::onEvent(NetworkEventCb cbEvent, const network_event event)
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

void NetworkSettingsClass::raiseEvent(const network_event event)
{
    for (uint32_t i = 0; i < _cbEventList.size(); i++) {
        const NetworkEventCbList_t entry = _cbEventList[i];
        if (entry.cb) {
            if (entry.event == event || entry.event == network_event::NETWORK_EVENT_MAX) {
                entry.cb(event);
            }
        }
    }
}

void NetworkSettingsClass::handleMDNS()
{
    const bool mdnsEnabled = Configuration.get().Mdns.Enabled;

    if (_lastMdnsEnabled == mdnsEnabled) {
        return;
    }

    _lastMdnsEnabled = mdnsEnabled;

    MDNS.end();

    if (!mdnsEnabled) {
        return;
    }

    if (MDNS.begin(getHostname())) {
        MessageOutput.print("MDNS responder starting...");

        MDNS.addService("http", "tcp", 80);
        MDNS.addService("opendtu", "tcp", 80);
        MDNS.addServiceTxt("opendtu", "tcp", "git_hash", __COMPILED_GIT_HASH__);

        MessageOutput.println("done");
    } else {
        MessageOutput.println("Error setting up MDNS responder!");
    }
}

void NetworkSettingsClass::setupMode()
{
    if (_adminEnabled) {
        WiFi.mode(WIFI_AP_STA);
        String ssidString = getApName();
        WiFi.softAPConfig(_apIp, _apIp, _apNetmask);
        WiFi.softAP(ssidString.c_str(), Configuration.get().Security.Password);
        _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
        _dnsServerStatus = true;
    } else {
        _dnsServerStatus = false;
        _dnsServer->stop();
        if (_networkMode == network_mode::WiFi) {
            WiFi.mode(WIFI_STA);
        } else {
            WiFi.mode(WIFI_MODE_NULL);
        }
    }

    if (PinMapping.isValidEthConfig()) {
        PinMapping_t& pin = PinMapping.get();
        ETH.begin(pin.eth_phy_addr, pin.eth_power, pin.eth_mdc, pin.eth_mdio, pin.eth_type, pin.eth_clk_mode);
    }
}

void NetworkSettingsClass::enableAdminMode()
{
    _adminEnabled = true;
    _adminTimeoutCounter = 0;
    _adminTimeoutCounterMax = Configuration.get().WiFi.ApTimeout * 60;
    setupMode();
}

String NetworkSettingsClass::getApName() const
{
    return String(ACCESS_POINT_NAME + String(Utils::getChipId()));
}

void NetworkSettingsClass::loop()
{
    if (_ethConnected) {
        if (_networkMode != network_mode::Ethernet) {
            // Do stuff when switching to Ethernet mode
            MessageOutput.println("Switch to Ethernet mode");
            _networkMode = network_mode::Ethernet;
            WiFi.mode(WIFI_MODE_NULL);
            setStaticIp();
            setHostname();
        }
    } else if (_networkMode != network_mode::WiFi) {
        // Do stuff when switching to Ethernet mode
        MessageOutput.println("Switch to WiFi mode");
        _networkMode = network_mode::WiFi;
        enableAdminMode();
        applyConfig();
    }

    if (millis() - _lastTimerCall > 1000) {
        if (_adminEnabled && _adminTimeoutCounterMax > 0) {
            _adminTimeoutCounter++;
            if (_adminTimeoutCounter % 10 == 0) {
                MessageOutput.printf("Admin AP remaining seconds: %d / %d\r\n", _adminTimeoutCounter, _adminTimeoutCounterMax);
            }
        }
        _connectTimeoutTimer++;
        _connectRedoTimer++;
        _lastTimerCall = millis();
    }
    if (_adminEnabled) {
        // Don't disable the admin mode when network is not available
        if (!isConnected()) {
            _adminTimeoutCounter = 0;
        }
        // If WiFi is connected to AP for more than adminTimeoutCounterMax
        // seconds, disable the internal Access Point
        if (_adminTimeoutCounter > _adminTimeoutCounterMax) {
            _adminEnabled = false;
            MessageOutput.println("Admin mode disabled");
            setupMode();
        }
        // It's nearly not possible to use the internal AP if the
        // WiFi is searching for an AP. So disable searching afer
        // WIFI_RECONNECT_TIMEOUT and repeat after WIFI_RECONNECT_REDO_TIMEOUT
        if (isConnected()) {
            _connectTimeoutTimer = 0;
            _connectRedoTimer = 0;
        } else {
            if (_connectTimeoutTimer > WIFI_RECONNECT_TIMEOUT && !_forceDisconnection) {
                MessageOutput.print("Disable search for AP... ");
                WiFi.mode(WIFI_AP);
                MessageOutput.println("done");
                _connectRedoTimer = 0;
                _forceDisconnection = true;
            }
            if (_connectRedoTimer > WIFI_RECONNECT_REDO_TIMEOUT && _forceDisconnection) {
                MessageOutput.print("Enable search for AP... ");
                WiFi.mode(WIFI_AP_STA);
                MessageOutput.println("done");
                applyConfig();
                _connectTimeoutTimer = 0;
                _forceDisconnection = false;
            }
        }
    }
    if (_dnsServerStatus) {
        _dnsServer->processNextRequest();
    }

    handleMDNS();
}

void NetworkSettingsClass::applyConfig()
{
    setHostname();
    if (!strcmp(Configuration.get().WiFi.Ssid, "")) {
        return;
    }
    MessageOutput.print("Configuring WiFi STA using ");
    if (strcmp(WiFi.SSID().c_str(), Configuration.get().WiFi.Ssid) || strcmp(WiFi.psk().c_str(), Configuration.get().WiFi.Password)) {
        MessageOutput.print("new credentials... ");
        WiFi.begin(
            Configuration.get().WiFi.Ssid,
            Configuration.get().WiFi.Password,
            WIFI_ALL_CHANNEL_SCAN);
    } else {
        MessageOutput.print("existing credentials... ");
        WiFi.begin();
    }
    MessageOutput.println("done");
    setStaticIp();
}

void NetworkSettingsClass::setHostname()
{
    MessageOutput.print("Setting Hostname... ");
    if (_networkMode == network_mode::WiFi) {
        if (WiFi.hostname(getHostname())) {
            MessageOutput.println("done");
        } else {
            MessageOutput.println("failed");
        }

        // Evil bad hack to get the hostname set up correctly
        WiFi.mode(WIFI_MODE_APSTA);
        WiFi.mode(WIFI_MODE_STA);
        setupMode();
    } else if (_networkMode == network_mode::Ethernet) {
        if (ETH.setHostname(getHostname().c_str())) {
            MessageOutput.println("done");
        } else {
            MessageOutput.println("failed");
        }
    }
}

void NetworkSettingsClass::setStaticIp()
{
    if (_networkMode == network_mode::WiFi) {
        if (Configuration.get().WiFi.Dhcp) {
            MessageOutput.print("Configuring WiFi STA DHCP IP... ");
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
            MessageOutput.println("done");
        } else {
            MessageOutput.print("Configuring WiFi STA static IP... ");
            WiFi.config(
                IPAddress(Configuration.get().WiFi.Ip),
                IPAddress(Configuration.get().WiFi.Gateway),
                IPAddress(Configuration.get().WiFi.Netmask),
                IPAddress(Configuration.get().WiFi.Dns1),
                IPAddress(Configuration.get().WiFi.Dns2));
            MessageOutput.println("done");
        }
    } else if (_networkMode == network_mode::Ethernet) {
        if (Configuration.get().WiFi.Dhcp) {
            MessageOutput.print("Configuring Ethernet DHCP IP... ");
            ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
            MessageOutput.println("done");
        } else {
            MessageOutput.print("Configuring Ethernet static IP... ");
            ETH.config(
                IPAddress(Configuration.get().WiFi.Ip),
                IPAddress(Configuration.get().WiFi.Gateway),
                IPAddress(Configuration.get().WiFi.Netmask),
                IPAddress(Configuration.get().WiFi.Dns1),
                IPAddress(Configuration.get().WiFi.Dns2));
            MessageOutput.println("done");
        }
    }
}

IPAddress NetworkSettingsClass::localIP() const
{
    switch (_networkMode) {
    case network_mode::Ethernet:
        return ETH.localIP();
        break;
    case network_mode::WiFi:
        return WiFi.localIP();
        break;
    default:
        return INADDR_NONE;
    }
}

IPAddress NetworkSettingsClass::subnetMask() const
{
    switch (_networkMode) {
    case network_mode::Ethernet:
        return ETH.subnetMask();
        break;
    case network_mode::WiFi:
        return WiFi.subnetMask();
        break;
    default:
        return IPAddress(255, 255, 255, 0);
    }
}

IPAddress NetworkSettingsClass::gatewayIP() const
{
    switch (_networkMode) {
    case network_mode::Ethernet:
        return ETH.gatewayIP();
        break;
    case network_mode::WiFi:
        return WiFi.gatewayIP();
        break;
    default:
        return INADDR_NONE;
    }
}

IPAddress NetworkSettingsClass::dnsIP(const uint8_t dns_no) const
{
    switch (_networkMode) {
    case network_mode::Ethernet:
        return ETH.dnsIP(dns_no);
        break;
    case network_mode::WiFi:
        return WiFi.dnsIP(dns_no);
        break;
    default:
        return INADDR_NONE;
    }
}

String NetworkSettingsClass::macAddress() const
{
    switch (_networkMode) {
    case network_mode::Ethernet:
        return ETH.macAddress();
        break;
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

    const uint32_t chipId = Utils::getChipId();
    snprintf(preparedHostname, WIFI_MAX_HOSTNAME_STRLEN + 1, config.WiFi.Hostname, chipId);

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

bool NetworkSettingsClass::isConnected() const
{
    return WiFi.localIP()[0] != 0 || ETH.localIP()[0] != 0;
}

network_mode NetworkSettingsClass::NetworkMode() const
{
    return _networkMode;
}

NetworkSettingsClass NetworkSettings;

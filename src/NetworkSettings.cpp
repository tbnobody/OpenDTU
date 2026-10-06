// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "NetworkSettings.h"
#include "Configuration.h"
#include "SyslogLogger.h"
#include "PinMapping.h"
#include "Utils.h"
#include "__compiled_constants.h"
#include "defaults.h"
#include <ESPmDNS.h>
#include <ETH.h>
#include <algorithm>
#include <esp_wifi.h>

#undef TAG
static const char* TAG = "network";

NetworkSettingsClass::NetworkSettingsClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&NetworkSettingsClass::loop, this))
    , _apIp(192, 168, 4, 1)
    , _apNetmask(255, 255, 255, 0)
    , _dnsServer(std::make_unique<DNSServer>())
{
}

void NetworkSettingsClass::init(Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

    WiFi.disconnect(true, true);

    WiFi.onEvent(std::bind(&NetworkSettingsClass::NetworkEvent, this, _1, _2));

    if (PinMapping.isValidW5500Config()) {
        const PinMapping_t& pin = PinMapping.get();
        _w5500 = W5500::setup(pin.w5500_mosi, pin.w5500_miso, pin.w5500_sclk, pin.w5500_cs, pin.w5500_int, pin.w5500_rst);
        if (_w5500)
            ESP_LOGI(TAG, "W5500: Connection successful");
        else
            ESP_LOGE(TAG, "W5500: Connection error!!");
    }
#if CONFIG_ETH_USE_ESP32_EMAC
    else if (PinMapping.isValidEthConfig()) {
        const PinMapping_t& pin = PinMapping.get();
#if ESP_ARDUINO_VERSION_MAJOR < 3
        ETH.begin(pin.eth_phy_addr, pin.eth_power, pin.eth_mdc, pin.eth_mdio, pin.eth_type, pin.eth_clk_mode);
#else
        ETH.begin(pin.eth_type, pin.eth_phy_addr, pin.eth_mdc, pin.eth_mdio, pin.eth_power, pin.eth_clk_mode);
#endif
    }
#endif

    setupMode();

    scheduler.addTask(_loopTask);
    _loopTask.enable();

    Syslog.init(scheduler);
}

void NetworkSettingsClass::NetworkEvent(const WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event) {
    case ARDUINO_EVENT_ETH_START:
        ESP_LOGI(TAG, "ETH start");
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_START);
        }
        break;
    case ARDUINO_EVENT_ETH_STOP:
        ESP_LOGI(TAG, "ETH stop");
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_STOP);
        }
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        ESP_LOGI(TAG, "ETH connected");
        _ethConnected = true;
        raiseEvent(network_event::NETWORK_CONNECTED);
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        ESP_LOGI(TAG, "ETH got IP: %s", ETH.localIP().toString().c_str());
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        ESP_LOGI(TAG, "ETH disconnected");
        _ethConnected = false;
        if (_networkMode == network_mode::Ethernet) {
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        ESP_LOGI(TAG, "WiFi connected");
        _stationAssociated = true;
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_CONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        // Reason codes can be found here: https://github.com/espressif/esp-idf/blob/5454d37d496a8c58542eb450467471404c606501/components/esp_wifi/include/esp_wifi_types_generic.h#L79-L141
        ESP_LOGW(TAG, "WiFi disconnected: %" PRIu8 "", info.wifi_sta_disconnected.reason);
        _stationAssociated = false;
        _stationDisconnectReason = info.wifi_sta_disconnected.reason;
        if (_networkMode == network_mode::WiFi) {
            // Deliberately stopping STA during the AP recovery window must not restart it.
            if (_performConnection && wifiConfigured()) {
                ESP_LOGI(TAG, "Try reconnecting");
                _lastReconnectAttempt = millis();
                cancelWifiScan();
                WiFi.disconnect(true, false);
                WiFi.begin();
            }
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        ESP_LOGI(TAG, "WiFi got ip: %s", WiFi.localIP().toString().c_str());
        _stationAssociated = true;
        _stationDisconnectReason = -1;
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        _stationAssociated = false;
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        onWifiScanDone(info.wifi_scan_done.status);
        break;
    default:
        break;
    }
}

bool NetworkSettingsClass::onEvent(DtuNetworkEventCb cbEvent, const network_event event)
{
    if (!cbEvent) {
        return pdFALSE;
    }
    DtuNetworkEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.event = event;
    _cbEventList.push_back(newEventHandler);
    return true;
}

void NetworkSettingsClass::raiseEvent(const network_event event)
{
    for (auto& entry : _cbEventList) {
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

    // Return if no state change
    if (_lastMdnsEnabled == mdnsEnabled) {
        return;
    }

    _lastMdnsEnabled = mdnsEnabled;
    MDNS.end();

    if (!mdnsEnabled) {
        ESP_LOGI(TAG, "MDNS disabled");
        return;
    }

    ESP_LOGI(TAG, "Starting MDNS responder...");

    if (!MDNS.begin(getHostname())) {
        ESP_LOGE(TAG, "Error setting up MDNS responder!");
        return;
    }

    MDNS.addService("http", "tcp", 80);
    MDNS.addService("opendtu", "tcp", 80);
    MDNS.addServiceTxt("opendtu", "tcp", "git_hash", __COMPILED_GIT_HASH__);

    ESP_LOGI(TAG, "MDNS started");
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
}

void NetworkSettingsClass::enableAdminMode()
{
    cancelWifiScan();
    restoreWifiScanMode();
    // This prevents a immediate "Disabling search for AP" when
    // the network connection persists for a long time and the
    // credentials gets changed.
    _connectTimeoutTimer = 0;
    _connectRedoTimer = 0;
    // Saving settings starts a fresh attempt even during the paused recovery window.
    _performConnection = true;

    _adminTimeoutCounter = 0;
    _adminTimeoutCounterMax = Configuration.get().WiFi.ApTimeout * 60;
    _adminEnabled = true;
    setupMode();
}

void NetworkSettingsClass::disableAdminMode()
{
    _adminEnabled = false;
    ESP_LOGI(TAG, "Admin mode disabled");
    setupMode();
}

bool NetworkSettingsClass::wifiConfigured() const
{
    // Check if SSID is empty
    return strcmp(Configuration.get().WiFi.Ssid, "");
}

String NetworkSettingsClass::getApName() const
{
    return String(ACCESS_POINT_NAME + String(Utils::getChipId()));
}

void NetworkSettingsClass::loop()
{
    if (_ethConnected) {
        if (_networkMode != network_mode::Ethernet) {
            cancelWifiScan();
            restoreWifiScanMode();
            // Do stuff when switching to Ethernet mode
            ESP_LOGI(TAG, "Switch to Ethernet mode");
            _networkMode = network_mode::Ethernet;
            WiFi.mode(WIFI_MODE_NULL);
            setStaticIp();
            setHostname();
        }
    } else if (_networkMode != network_mode::WiFi) {
        // Do stuff when switching to Ethernet mode
        ESP_LOGI(TAG, "Switch to WiFi mode");
        _networkMode = network_mode::WiFi;
        enableAdminMode();
        applyConfig();
    }

    processWifiScan();
    const bool scanInProgress = _wifiScanActive && !_wifiScanCancelled;

    if (millis() - _lastTimerCall > 1000) {
        if (_adminEnabled && _adminTimeoutCounterMax > 0) {
            _adminTimeoutCounter++;
            if (_adminTimeoutCounter % 10 == 0) {
                ESP_LOGI(TAG, "Admin AP remaining seconds: %" PRIu32 " / %" PRIu32 "", _adminTimeoutCounter, _adminTimeoutCounterMax);
            }
        }
        if (!scanInProgress && _performConnection && !isConnected() && wifiConfigured() && millis() - _lastReconnectAttempt > 60000) {
            ESP_LOGW(TAG, "Wifi reconnect watchdog triggered... Resetting Wifi hardware");
            WiFi.disconnect(true, false);
            WiFi.mode(WIFI_MODE_NULL);
            if (_adminEnabled) {
                // Call enableAdminMode to reset all the timeout values.
                // Otherwise the search for AP gets disabled immediatly after wifi reset.
                enableAdminMode();
            }
            applyConfig();
            _lastReconnectAttempt = millis(); // Just in case if the reconnect method gets not triggered
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
        if (!scanInProgress && _adminTimeoutCounter > _adminTimeoutCounterMax) {
            disableAdminMode();
        }
        // It's nearly not possible to use the internal AP if the
        // WiFi is searching for an AP. So disable searching afer
        // WIFI_RECONNECT_TIMEOUT and repeat after WIFI_RECONNECT_REDO_TIMEOUT
        if (isConnected()) {
            _connectTimeoutTimer = 0;
            _connectRedoTimer = 0;
        } else if (!scanInProgress) {
            if (_connectTimeoutTimer > WIFI_RECONNECT_TIMEOUT && _performConnection) {
                ESP_LOGI(TAG, "Disabling search for AP...");
                _connectRedoTimer = 0;
                _performConnection = false;
                WiFi.mode(WIFI_AP);
            }
            if (_connectRedoTimer > WIFI_RECONNECT_REDO_TIMEOUT && !_performConnection) {
                ESP_LOGI(TAG, "Enable search for AP...");
                _performConnection = true;
                WiFi.mode(WIFI_AP_STA);
                applyConfig();
                _connectTimeoutTimer = 0;
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
    cancelWifiScan();
    restoreWifiScanMode();
    _stationDisconnectReason = -1;
    setHostname();

    const auto& config = Configuration.get().WiFi;

    if (!wifiConfigured()) {
        return;
    }

    const bool newCredentials = strcmp(WiFi.SSID().c_str(), config.Ssid) || strcmp(WiFi.psk().c_str(), config.Password);

    ESP_LOGI(TAG, "Start configuring WiFi STA using %s credentials",
        newCredentials ? "new" : "existing");

    bool success = false;
    if (newCredentials) {
        success = WiFi.begin(
            config.Ssid,
            config.Password) != WL_CONNECT_FAILED;
    } else {
        success = WiFi.begin() != WL_CONNECT_FAILED;
    }

    ESP_LOG_LEVEL_LOCAL((success ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "Configuring WiFi %s", success ? "done" : "failed");

    setStaticIp();

    Syslog.updateSettings(getHostname());
}

void NetworkSettingsClass::setHostname()
{
    if (_networkMode == network_mode::Undefined) {
        return;
    }

    const String hostname = getHostname();
    bool success = false;

    ESP_LOGI(TAG, "Start setting hostname...");
    if (_networkMode == network_mode::WiFi) {
        success = WiFi.hostname(hostname);

        // Evil bad hack to get the hostname set up correctly
        WiFi.mode(WIFI_MODE_APSTA);
        WiFi.mode(WIFI_MODE_STA);
        setupMode();
    } else if (_networkMode == network_mode::Ethernet) {
        success = ETH.setHostname(hostname.c_str());
    }

    ESP_LOG_LEVEL_LOCAL((success ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "Setting hostname %s", success ? "done" : "failed");
}

void NetworkSettingsClass::setStaticIp()
{
    if (_networkMode == network_mode::Undefined) {
        return;
    }

    const auto& config = Configuration.get().WiFi;
    const char* mode = (_networkMode == network_mode::WiFi) ? "WiFi" : "Ethernet";
    const char* ipType = config.Dhcp ? "DHCP" : "static";

    ESP_LOGI(TAG, "Start configuring %s %s IP...", mode, ipType);

    bool success = false;
    if (_networkMode == network_mode::WiFi) {
        if (config.Dhcp) {
            success = WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        } else {
            success = WiFi.config(
                IPAddress(config.Ip),
                IPAddress(config.Gateway),
                IPAddress(config.Netmask),
                IPAddress(config.Dns1),
                IPAddress(config.Dns2));
        }
    } else if (_networkMode == network_mode::Ethernet) {
        if (config.Dhcp) {
            success = ETH.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        } else {
            success = ETH.config(
                IPAddress(config.Ip),
                IPAddress(config.Gateway),
                IPAddress(config.Netmask),
                IPAddress(config.Dns1),
                IPAddress(config.Dns2));
        }
    }

    ESP_LOG_LEVEL_LOCAL((success ? ESP_LOG_INFO : ESP_LOG_ERROR), TAG, "Configure IP %s", success ? "done" : "failed");
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
        if (_w5500) {
            return _w5500->macAddress();
        }
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
    return (WiFi.localIP()[0] != 0 && WiFi.isConnected() ) || ETH.localIP()[0] != 0;
}

network_mode NetworkSettingsClass::NetworkMode() const
{
    return _networkMode;
}

const char* NetworkSettingsClass::getStationConnectionState() const
{
    if (_networkMode == network_mode::Ethernet) {
        return "disabled";
    }
    if (!wifiConfigured()) {
        return "not_configured";
    }
    if (WiFi.isConnected() && WiFi.localIP()[0] != 0) {
        return "connected";
    }
    if (!_performConnection) {
        return "paused";
    }
    if ((WiFi.getMode() & WIFI_STA) == 0) {
        return "disabled";
    }
    // Association precedes DHCP; losing an IP does not end the Wi-Fi association.
    return _stationAssociated ? "waiting_for_ip" : "connecting";
}

int NetworkSettingsClass::getStationDisconnectReason() const
{
    return _stationDisconnectReason;
}

int NetworkSettingsClass::getStationRetryIn() const
{
    if (strcmp(getStationConnectionState(), "paused") != 0) {
        return -1;
    }
    const uint32_t elapsed = _connectRedoTimer;
    return elapsed < WIFI_RECONNECT_REDO_TIMEOUT ? WIFI_RECONNECT_REDO_TIMEOUT - elapsed : 0;
}

void NetworkSettingsClass::requestWifiScan()
{
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    if (_wifiScanActive || strcmp(_wifiScanStatus.state, "running") == 0) {
        return;
    }
    _wifiScanStatus = {};
    _wifiScanStatus.state = "running";
    _wifiScanRequested = true;
}

WifiScanStatus NetworkSettingsClass::getWifiScanStatus() const
{
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    return _wifiScanStatus;
}

void NetworkSettingsClass::restoreWifiScanMode()
{
    // A scan can temporarily enable STA in the AP-only recovery window. It must
    // not turn that into a connection attempt or discard the existing retry timer.
    if (_wifiScanRestoreSta) {
        WiFi.enableSTA(false);
        _wifiScanRestoreSta = false;
    }
}

void NetworkSettingsClass::cancelWifiScan()
{
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    if (_wifiScanActive && !_wifiScanCancelled.exchange(true)) {
        _wifiScanStatus.state = "failed";
        _wifiScanStatus.error = "busy";
        _wifiScanStatus.count = 0;
        // Stop before a disconnect handler resets the driver. Keep ownership
        // until SCAN_DONE releases Arduino's results; a new scan must wait.
        esp_wifi_scan_stop();
    }
}

void NetworkSettingsClass::processWifiScan()
{
    constexpr uint32_t timeout = 10000;
    constexpr uint32_t cacheLifetime = 60000;
    const uint32_t now = millis();

    if (_wifiScanActive) {
        if (_wifiScanCancelled) {
            restoreWifiScanMode();
        }
        if (_wifiScanCompleted.exchange(false)) {
            restoreWifiScanMode();
            std::lock_guard<std::mutex> lock(_wifiScanMutex);
            _wifiScanStatus.state = _wifiScanStatus.error ? "failed" : "complete";
            _wifiScanFinished = now;
            _wifiScanActive = false;
        } else if (!_wifiScanCancelled && now - _wifiScanStarted >= timeout) {
            cancelWifiScan();
            restoreWifiScanMode();
            std::lock_guard<std::mutex> lock(_wifiScanMutex);
            _wifiScanStatus.error = "timeout";
        }
        return;
    }

    if (_wifiScanRequested.exchange(false)) {
        // The driver rejects scans while joining. Never disconnect a working
        // station or change its credentials just to populate the SSID picker.
        if (_networkMode == network_mode::Ethernet || (wifiConfigured() && _performConnection && !_stationAssociated)) {
            std::lock_guard<std::mutex> lock(_wifiScanMutex);
            _wifiScanStatus.state = "failed";
            _wifiScanStatus.error = _networkMode == network_mode::Ethernet ? "wifi_disabled" : "busy";
            _wifiScanFinished = now;
            return;
        }
        _wifiScanRestoreSta = (WiFi.getMode() & WIFI_STA) == 0;
        _wifiScanCancelled = false;
        _wifiScanCompleted = false;
        _wifiScanActive = true;
        _wifiScanStarted = now;
        if (WiFi.scanNetworks(true, false, false, 300) == WIFI_SCAN_FAILED) {
            restoreWifiScanMode();
            std::lock_guard<std::mutex> lock(_wifiScanMutex);
            _wifiScanStatus.state = "failed";
            _wifiScanStatus.error = "scan_failed";
            _wifiScanFinished = now;
            _wifiScanActive = false;
        }
        return;
    }

    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    if (!_wifiScanRequested && now - _wifiScanFinished >= cacheLifetime) {
        _wifiScanStatus = {};
    }
}

void NetworkSettingsClass::onWifiScanDone(uint32_t status)
{
    // Arduino fills its result buffer before this callback. Copy and release it
    // here so timeout handling cannot free a buffer being filled on this task.
    if (_wifiScanActive && !_wifiScanCancelled) {
        std::lock_guard<std::mutex> lock(_wifiScanMutex);
        const int count = WiFi.scanComplete();
        if (status != 0 || count < 0) {
            _wifiScanStatus.error = "scan_failed";
        } else {
            auto& result = _wifiScanStatus;
            for (int i = 0; i < count; ++i) {
                const auto* ap = static_cast<const wifi_ap_record_t*>(WiFi.getScanInfoByIndex(i));
                if (!ap || ap->ssid[0] == 0) {
                    continue;
                }
                const char* ssid = reinterpret_cast<const char*>(ap->ssid);
                size_t index = 0;
                while (index < result.count && strcmp(result.networks[index].ssid, ssid) != 0) {
                    ++index;
                }
                if (index < result.count && result.networks[index].rssi >= ap->rssi) {
                    continue;
                }
                if (index == result.networks.size()) {
                    index--;
                    if (result.networks[index].rssi >= ap->rssi) {
                        continue;
                    }
                } else if (index == result.count) {
                    result.count++;
                }
                strlcpy(result.networks[index].ssid, ssid, sizeof(result.networks[index].ssid));
                result.networks[index].rssi = ap->rssi;
                result.networks[index].secure = ap->authmode != WIFI_AUTH_OPEN;
                std::sort(result.networks.begin(), result.networks.begin() + result.count, [](const auto& left, const auto& right) {
                    return left.rssi != right.rssi ? left.rssi > right.rssi : strcmp(left.ssid, right.ssid) < 0;
                });
            }
        }
    }
    WiFi.scanDelete();
    if (_wifiScanActive) {
        _wifiScanCompleted = true;
    }
}

NetworkSettingsClass NetworkSettings;

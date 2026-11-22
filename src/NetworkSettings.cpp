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
#include <array>
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
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_CONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        // Reason codes can be found here: https://github.com/espressif/esp-idf/blob/5454d37d496a8c58542eb450467471404c606501/components/esp_wifi/include/esp_wifi_types_generic.h#L79-L141
        ESP_LOGW(TAG, "WiFi disconnected: %" PRIu8 "", info.wifi_sta_disconnected.reason);
        if (_networkMode == network_mode::WiFi) {
            // AP-only recovery must not restart a paused connection.
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
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
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
        // The scheduler starts this once association is no longer in progress.
        _wifiScanPrewarmPending = true;
        String ssidString = getApName();
        WiFi.softAPConfig(_apIp, _apIp, _apNetmask);
        WiFi.softAP(ssidString.c_str(), Configuration.get().Security.Password);
        _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
        _dnsServerStatus = true;
    } else {
        _wifiScanPrewarmPending = false;
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
                // Publish the pause before mode changes can emit disconnect events.
                _performConnection = false;
                WiFi.mode(WIFI_AP);
                _connectRedoTimer = 0;
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

String NetworkSettingsClass::buildHostname(const String& hostTemplate)
{
    const uint32_t chipId = Utils::getChipId();
    char chipIdStr[7];
    snprintf(chipIdStr, sizeof(chipIdStr), "%06X", static_cast<unsigned>(chipId));

    String prepared = hostTemplate;
    prepared.replace("$id$", chipIdStr);
    return prepared;
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

    String prepared = buildHostname(config.WiFi.Hostname);
    strlcpy(preparedHostname, prepared.c_str(), sizeof(preparedHostname));

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
        String prepared = buildHostname(APP_HOSTNAME);
        strlcpy(resultHostname, prepared.c_str(), sizeof(preparedHostname));
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

void NetworkSettingsClass::requestWifiScan(const bool refresh)
{
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    if (_wifiScanActive || _wifiScanRequested || _wifiScanRetryPending) {
        return;
    }
    if (!refresh && strcmp(_wifiScanState, "idle") != 0) {
        return;
    }
    _wifiScanState = "running";
    _wifiScanError = nullptr;
    _wifiScanRetries = 0;
    _wifiScanRequested = true;
}

bool NetworkSettingsClass::getWifiScanStatus(JsonVariant root) const
{
    // Starting/deleting a scan uses the same lock. Arduino has already finished
    // writing its buffer before the scheduler publishes the complete state.
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    root["state"] = _wifiScanState;
    root["error"] = _wifiScanError;
    JsonArray networks = root["networks"].to<JsonArray>();
    if (strcmp(_wifiScanState, "complete") != 0) {
        return strcmp(_wifiScanState, "running") == 0;
    }

    // Only indices are temporary (128 bytes), not another copy of the AP records.
    // Keep separate BSSIDs for mesh networks and select the strongest 32 results.
    std::array<int, 32> indices {};
    size_t count = 0;
    const int available = WiFi.scanComplete();
    for (int i = 0; i < available; ++i) {
        const auto* ap = static_cast<const wifi_ap_record_t*>(WiFi.getScanInfoByIndex(i));
        if (!ap || ap->ssid[0] == 0) {
            continue;
        }
        size_t pos = 0;
        while (pos < count) {
            const auto* other = static_cast<const wifi_ap_record_t*>(WiFi.getScanInfoByIndex(indices[pos]));
            if (ap->rssi > other->rssi || (ap->rssi == other->rssi && memcmp(ap->bssid, other->bssid, 6) < 0)) {
                break;
            }
            ++pos;
        }
        if (pos == indices.size()) {
            continue;
        }
        if (count < indices.size()) {
            ++count;
        }
        for (size_t j = count - 1; j > pos; --j) {
            indices[j] = indices[j - 1];
        }
        indices[pos] = i;
    }
    for (size_t i = 0; i < count; ++i) {
        const auto* ap = static_cast<const wifi_ap_record_t*>(WiFi.getScanInfoByIndex(indices[i]));
        JsonObject network = networks.add<JsonObject>();
        // Own these strings before releasing the buffer-lifetime lock.
        network["ssid"] = String(reinterpret_cast<const char*>(ap->ssid));
        char bssid[18];
        snprintf(bssid, sizeof(bssid), "%02X:%02X:%02X:%02X:%02X:%02X",
            ap->bssid[0], ap->bssid[1], ap->bssid[2], ap->bssid[3], ap->bssid[4], ap->bssid[5]);
        network["bssid"] = String(bssid);
        network["rssi"] = ap->rssi;
        network["secure"] = ap->authmode != WIFI_AUTH_OPEN;
    }
    return false;
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

void NetworkSettingsClass::cancelWifiScan(const char* error)
{
    std::lock_guard<std::mutex> lock(_wifiScanMutex);
    _wifiScanRequested = false;
    _wifiScanRetryPending = false;
    if (_wifiScanActive) {
        _wifiScanState = "failed";
        _wifiScanError = error;
        // Stop before a disconnect handler resets the driver. Keep ownership
        // until SCAN_DONE has finished filling Arduino's result buffer.
        if (!_wifiScanCancelled.exchange(true)) {
            esp_wifi_scan_stop();
        }
    } else {
        WiFi.scanDelete();
        _wifiScanState = "idle";
        _wifiScanError = nullptr;
    }
}

void NetworkSettingsClass::finishWifiScan(const char* error, const uint32_t now)
{
    // Called by the scheduler with _wifiScanMutex held and no driver writer left.
    _wifiScanActive = false;
    _wifiScanFinished = now;
    _wifiScanRetryPending = error && (strcmp(error, "scan_failed") == 0 || strcmp(error, "timeout") == 0)
        && _wifiScanRetries == 0;
    if (_wifiScanRetryPending) {
        ++_wifiScanRetries;
    }
    _wifiScanState = _wifiScanRetryPending ? "running" : error ? "failed" : "complete";
    _wifiScanError = _wifiScanRetryPending ? nullptr : error;
    if (error) {
        WiFi.scanDelete();
    }
}

void NetworkSettingsClass::processWifiScan()
{
    constexpr uint32_t timeout = 10000;
    constexpr uint32_t cacheLifetime = 60000;
    constexpr uint32_t retryDelay = 1000;
    const uint32_t now = millis();
    std::lock_guard<std::mutex> lock(_wifiScanMutex);

    if (_wifiScanActive) {
        if (_wifiScanCancelled) {
            restoreWifiScanMode();
        }
        if (_wifiScanCompleted.exchange(false)) {
            const char* error = _wifiScanCancelled ? _wifiScanError
                : (_wifiScanCompletionStatus != 0 || WiFi.scanComplete() < 0) ? "scan_failed" : nullptr;
            restoreWifiScanMode();
            finishWifiScan(error, now);
        } else if (!_wifiScanCancelled && now - _wifiScanStarted >= timeout) {
            _wifiScanState = _wifiScanRetries == 0 ? "running" : "failed";
            _wifiScanError = "timeout";
            _wifiScanCancelled = true;
            esp_wifi_scan_stop();
            restoreWifiScanMode();
        }
        return;
    }

    const bool canScan = _networkMode == network_mode::WiFi
        && (!wifiConfigured() || !_performConnection || WiFi.isConnected());
    if (_wifiScanPrewarmPending && canScan) {
        _wifiScanPrewarmPending = false;
        if (strcmp(_wifiScanState, "idle") == 0
            || (_wifiScanError && strcmp(_wifiScanError, "busy") == 0)) {
            _wifiScanState = "running";
            _wifiScanError = nullptr;
            _wifiScanRetries = 0;
            _wifiScanRequested = true;
        }
    }
    if (_wifiScanRetryPending && now - _wifiScanFinished >= retryDelay) {
        _wifiScanRetryPending = false;
        _wifiScanRequested = true;
    }
    if (_wifiScanRequested.exchange(false)) {
        // The driver rejects scans while joining. Never disconnect a working
        // station or change its credentials just to populate the SSID picker.
        if (!canScan) {
            finishWifiScan(_networkMode == network_mode::Ethernet ? "wifi_disabled" : "busy", now);
            return;
        }
        _wifiScanRestoreSta = (WiFi.getMode() & WIFI_STA) == 0;
        _wifiScanCancelled = false;
        _wifiScanCompleted = false;
        _wifiScanActive = true;
        _wifiScanStarted = now;
        if (WiFi.scanNetworks(true, false, false, 300) == WIFI_SCAN_FAILED) {
            restoreWifiScanMode();
            finishWifiScan("scan_failed", now);
        }
        return;
    }

    if (!_wifiScanRetryPending && strcmp(_wifiScanState, "idle") != 0 && now - _wifiScanFinished >= cacheLifetime) {
        WiFi.scanDelete();
        _wifiScanState = "idle";
        _wifiScanError = nullptr;
    }
}

void NetworkSettingsClass::onWifiScanDone(uint32_t status)
{
    // Arduino has populated its buffer before this notification. Do not lock,
    // inspect, copy or free results on the Arduino event task.
    if (_wifiScanActive) {
        _wifiScanCompletionStatus = status;
        _wifiScanCompleted = true;
    }
}

NetworkSettingsClass NetworkSettings;

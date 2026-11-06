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
            _lastReconnectAttempt = millis();
            if (_wifiRescanSwitching && info.wifi_sta_disconnected.reason == WIFI_REASON_ASSOC_LEAVE) {
                // we left the access point on purpose, the connection
                // attempt to the better access point is already in progress
                _wifiRescanSwitching = false;
                ESP_LOGI(TAG, "Left access point to connect to a better one");
            } else if (isWifiBssidPinned()) {
                // the BSSID was pinned by the WiFi rescan. do not insist on
                // that access point, but connect to any with the same SSID.
                ESP_LOGI(TAG, "Try reconnecting to any access point");
                auto const& config = Configuration.get().WiFi;
                WiFi.disconnect(true, false);
                WiFi.begin(config.Ssid, config.Password);
            } else {
                ESP_LOGI(TAG, "Try reconnecting");
                WiFi.disconnect(true, false);
                WiFi.begin();
            }
            raiseEvent(network_event::NETWORK_DISCONNECTED);
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _wifiRescanSwitching = false;
        ESP_LOGI(TAG, "WiFi got ip: %s", WiFi.localIP().toString().c_str());
        if (_networkMode == network_mode::WiFi) {
            raiseEvent(network_event::NETWORK_GOT_IP);
        }
        break;
    default:
        break;
    }
}

bool NetworkSettingsClass::onEvent(DtuNetworkEventCb cbEvent, const network_event event, const String &name)
{
    if (!cbEvent) { return pdFALSE; }

    if (!name.isEmpty()) {
        for (auto it = _cbEventList.begin(); it != _cbEventList.end(); ++it) {
            if (it->name != name) { continue; }

            ESP_LOGE(TAG, "Event with name '%s' already registered!", name.c_str());
            return pdFALSE;
        }
    }

    DtuNetworkEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.event = event;
    newEventHandler.name = name;

    ESP_LOGD(TAG, "Registering event: '%s'", name.c_str());

    _cbEventList.push_back(newEventHandler);
    return true;
}

void NetworkSettingsClass::deregisterEvent(const String &name)
{
    if (name.isEmpty()) { return; }

    for (auto it = _cbEventList.begin(); it != _cbEventList.end(); ++it) {
        if (it->name != name) { continue; }

        ESP_LOGD(TAG, "Deregistering event: '%s'", name.c_str());
        _cbEventList.erase(it);
        break;
    }
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
    // This prevents a immediate "Disabling search for AP" when
    // the network connection persists for a long time and the
    // credentials gets changed.
    _connectTimeoutTimer = 0;
    _connectRedoTimer = 0;

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

    if (millis() - _lastTimerCall > 1000) {
        if (_adminEnabled && _adminTimeoutCounterMax > 0) {
            _adminTimeoutCounter++;
            if (_adminTimeoutCounter % 10 == 0) {
                ESP_LOGI(TAG, "Admin AP remaining seconds: %" PRIu32 " / %" PRIu32 "", _adminTimeoutCounter, _adminTimeoutCounterMax);
            }
        }
        if (_performConnection && !isConnected() && wifiConfigured() && millis() - _lastReconnectAttempt > 60000) {
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
        if (_adminTimeoutCounter > _adminTimeoutCounterMax) {
            disableAdminMode();
        }
        // It's nearly not possible to use the internal AP if the
        // WiFi is searching for an AP. So disable searching afer
        // WIFI_RECONNECT_TIMEOUT and repeat after WIFI_RECONNECT_REDO_TIMEOUT
        if (isConnected()) {
            _connectTimeoutTimer = 0;
            _connectRedoTimer = 0;
        } else {
            if (_connectTimeoutTimer > WIFI_RECONNECT_TIMEOUT && _performConnection) {
                ESP_LOGI(TAG, "Disabling search for AP...");
                WiFi.mode(WIFI_AP);
                _connectRedoTimer = 0;
                _performConnection = false;
            }
            if (_connectRedoTimer > WIFI_RECONNECT_REDO_TIMEOUT && !_performConnection) {
                ESP_LOGI(TAG, "Enable search for AP...");
                WiFi.mode(WIFI_AP_STA);
                applyConfig();
                _connectTimeoutTimer = 0;
                _performConnection = true;
            }
        }
    }
    if (_dnsServerStatus) {
        _dnsServer->processNextRequest();
    }

    handleMDNS();
    handleWifiRescan();
}

bool NetworkSettingsClass::isWifiBssidPinned()
{
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        return false;
    }

    wifi_config_t conf;
    if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
        return false;
    }

    return conf.sta.bssid_set;
}

// periodically scans for access points broadcasting the configured SSID and
// switches to one with a considerably stronger signal. inspired by Tasmota.
void NetworkSettingsClass::handleWifiRescan()
{
    auto const& config = Configuration.get().WiFi;

    // scanning while the admin access point is active would disturb it.
    // with a timeout of 0, the admin access point is never closed, though,
    // so scan as long as no client is connected to it.
    bool active = config.RescanEnabled
        && _networkMode == network_mode::WiFi
        && (!_adminEnabled || (config.ApTimeout == 0 && WiFi.softAPgetStationNum() == 0))
        && WiFi.isConnected();

    if (_wifiRescanRunning) {
        int16_t result = WiFi.scanComplete();
        if (result == WIFI_SCAN_RUNNING) { return; }

        _wifiRescanRunning = false;
        _lastWifiRescan = millis();

        if (result < 0) {
            ESP_LOGW(TAG, "WiFi rescan failed");
        } else if (active) {
            evaluateWifiRescan(result);
        }

        WiFi.scanDelete();
        return;
    }

    if (!active) {
        // start counting the interval once we are (re-)connected
        _lastWifiRescan = millis();
        return;
    }

    uint32_t const intervalMillis = std::max<uint32_t>(config.RescanInterval, 5) * 60 * 1000;
    if (millis() - _lastWifiRescan < intervalMillis) {
        return;
    }

    ESP_LOGI(TAG, "Scanning for a better access point...");
    if (WiFi.scanNetworks(true/*async*/, false/*show_hidden*/, false/*passive*/,
            300/*max_ms_per_chan*/, 0/*channel*/, config.Ssid) != WIFI_SCAN_RUNNING) {
        ESP_LOGW(TAG, "Failed to start WiFi rescan");
        _lastWifiRescan = millis();
        return;
    }

    _wifiRescanRunning = true;
}

void NetworkSettingsClass::evaluateWifiRescan(int16_t networkCount)
{
    auto const& config = Configuration.get().WiFi;

    uint8_t const* currentBssidPtr = WiFi.BSSID();
    if (currentBssidPtr == nullptr) { return; }

    uint8_t currentBssid[6];
    memcpy(currentBssid, currentBssidPtr, sizeof(currentBssid));
    int32_t const currentRssi = WiFi.RSSI();

    // a different access point must exceed this RSSI to be switched to
    int32_t bestRssi = currentRssi + config.RescanThreshold;
    int16_t bestNetwork = -1;

    for (int16_t i = 0; i < networkCount; ++i) {
        String ssid;
        uint8_t encryptionType;
        int32_t rssi;
        uint8_t* bssid;
        int32_t channel;
        if (!WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel)) { continue; }

        if (ssid != config.Ssid) { continue; }

        bool const isCurrent = memcmp(bssid, currentBssid, sizeof(currentBssid)) == 0;

        ESP_LOGD(TAG, "Found access point %s on channel %" PRId32 " with RSSI %" PRId32 " dBm%s",
            WiFi.BSSIDstr(i).c_str(), channel, rssi, (isCurrent ? " (current)" : ""));

        if (isCurrent || rssi <= bestRssi) { continue; }

        bestRssi = rssi;
        bestNetwork = i;
    }

    if (bestNetwork < 0) {
        ESP_LOGI(TAG, "No access point with a considerably stronger signal than "
            "the current one (%s, %" PRId32 " dBm) found",
            WiFi.BSSIDstr().c_str(), currentRssi);
        return;
    }

    ESP_LOGI(TAG, "Switching from access point %s (%" PRId32 " dBm) to %s (%" PRId32 " dBm) on channel %" PRId32,
        WiFi.BSSIDstr().c_str(), currentRssi,
        WiFi.BSSIDstr(bestNetwork).c_str(), bestRssi, WiFi.channel(bestNetwork));

    // pinning the BSSID makes sure to connect to the selected access point.
    // the pin is removed on the next disconnect, see NetworkEvent().
    _wifiRescanSwitching = true;
    if (WiFi.begin(config.Ssid, config.Password, WiFi.channel(bestNetwork),
            WiFi.BSSID(bestNetwork)) == WL_CONNECT_FAILED) {
        ESP_LOGE(TAG, "Failed to switch access point");
        _wifiRescanSwitching = false;
    }
}

void NetworkSettingsClass::applyConfig()
{
    setHostname();

    // syslog is also used without WiFi (e.g., Ethernet only), so configure it
    // before bailing out below.
    Syslog.updateSettings(getHostname());

    const auto& config = Configuration.get().WiFi;

    if (!wifiConfigured()) {
        return;
    }

    // a BSSID pinned by the WiFi rescan must not be re-used, as we would
    // then never connect to any other access point with the same SSID.
    const bool newCredentials = strcmp(WiFi.SSID().c_str(), config.Ssid) || strcmp(WiFi.psk().c_str(), config.Password) || isWifiBssidPinned();

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
    // a static Ethernet IP is still reported after the link went down
    return (WiFi.localIP()[0] != 0 && WiFi.isConnected() ) || (_ethConnected && ETH.localIP()[0] != 0);
}

network_mode NetworkSettingsClass::NetworkMode() const
{
    return _networkMode;
}

NetworkSettingsClass NetworkSettings;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "W5500.h"
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <TaskSchedulerDeclarations.h>
#include <WiFi.h>
#include <atomic>
#include <mutex>
#include <vector>

enum class network_mode {
    WiFi,
    Ethernet,
    Undefined
};

enum class network_event {
    NETWORK_UNKNOWN,
    NETWORK_START,
    NETWORK_STOP,
    NETWORK_CONNECTED,
    NETWORK_DISCONNECTED,
    NETWORK_GOT_IP,
    NETWORK_LOST_IP,
    NETWORK_EVENT_MAX
};

typedef std::function<void(network_event event)> DtuNetworkEventCb;

typedef struct DtuNetworkEventCbList {
    DtuNetworkEventCb cb;
    network_event event;

    DtuNetworkEventCbList()
        : cb(nullptr)
        , event(network_event::NETWORK_UNKNOWN)
    {
    }
} DtuNetworkEventCbList_t;

class NetworkSettingsClass {
public:
    NetworkSettingsClass();
    void init(Scheduler& scheduler);
    void applyConfig();
    void enableAdminMode();
    String getApName() const;

    IPAddress localIP() const;
    IPAddress subnetMask() const;
    IPAddress gatewayIP() const;
    IPAddress dnsIP(const uint8_t dns_no = 0) const;
    String macAddress() const;
    static String getHostname();
    bool isConnected() const;
    network_mode NetworkMode() const;
    void requestWifiScan(bool refresh = true);
    // Copy ready Arduino-owned results directly into the response. True means pending.
    bool getWifiScanStatus(JsonVariant root) const;

    bool onEvent(DtuNetworkEventCb cbEvent, const network_event event = network_event::NETWORK_EVENT_MAX);
    void raiseEvent(const network_event event);

private:
    void loop();
    void setHostname();
    void setStaticIp();
    void handleMDNS();
    void setupMode();
    void NetworkEvent(const WiFiEvent_t event, WiFiEventInfo_t info);
    void disableAdminMode();
    bool wifiConfigured() const;
    void processWifiScan();
    void onWifiScanDone(uint32_t status);
    void cancelWifiScan(const char* error = "busy");
    void finishWifiScan(const char* error, uint32_t now);
    void restoreWifiScanMode();

    static String buildHostname(const String& hostTemplate);

    Task _loopTask;

    static constexpr byte DNS_PORT = 53;

    bool _adminEnabled = true;
    std::atomic<bool> _performConnection { true };
    uint32_t _adminTimeoutCounter = 0;
    uint32_t _adminTimeoutCounterMax = 0;
    uint32_t _connectTimeoutTimer = 0;
    uint32_t _connectRedoTimer = 0;
    uint32_t _lastReconnectAttempt = 0;
    uint32_t _lastTimerCall = 0;
    IPAddress _apIp;
    IPAddress _apNetmask;
    std::unique_ptr<DNSServer> _dnsServer;
    bool _dnsServerStatus = false;
    network_mode _networkMode = network_mode::Undefined;
    bool _ethConnected = false;
    std::vector<DtuNetworkEventCbList_t> _cbEventList;
    bool _lastMdnsEnabled = false;
    std::unique_ptr<W5500> _w5500;
    mutable std::mutex _wifiScanMutex;
    const char* _wifiScanState = "idle";
    const char* _wifiScanError = nullptr;
    std::atomic<bool> _wifiScanRequested { false };
    std::atomic<bool> _wifiScanActive { false };
    std::atomic<bool> _wifiScanCompleted { false };
    std::atomic<uint32_t> _wifiScanCompletionStatus { 0 };
    std::atomic<bool> _wifiScanCancelled { false };
    bool _wifiScanRestoreSta = false;
    bool _wifiScanPrewarmPending = false;
    bool _wifiScanRetryPending = false;
    uint8_t _wifiScanRetries = 0;
    uint32_t _wifiScanStarted = 0;
    uint32_t _wifiScanFinished = 0;
};

extern NetworkSettingsClass NetworkSettings;

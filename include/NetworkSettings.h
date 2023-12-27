// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <DNSServer.h>
#include <TaskSchedulerDeclarations.h>
#include <WiFi.h>
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

typedef std::function<void(network_event event)> NetworkEventCb;

typedef struct NetworkEventCbList {
    NetworkEventCb cb;
    network_event event;

    NetworkEventCbList()
        : cb(nullptr)
        , event(network_event::NETWORK_UNKNOWN)
    {
    }
} NetworkEventCbList_t;

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

    bool onEvent(NetworkEventCb cbEvent, const network_event event = network_event::NETWORK_EVENT_MAX);
    void raiseEvent(const network_event event);

private:
    void loop();
    void setHostname();
    void setStaticIp();
    void handleMDNS();
    void setupMode();
    void NetworkEvent(const WiFiEvent_t event);

    Task _loopTask;

    static constexpr byte DNS_PORT = 53;

    bool _adminEnabled = true;
    bool _forceDisconnection = false;
    uint32_t _adminTimeoutCounter = 0;
    uint32_t _adminTimeoutCounterMax = 0;
    uint32_t _connectTimeoutTimer = 0;
    uint32_t _connectRedoTimer = 0;
    uint32_t _lastTimerCall = 0;
    IPAddress _apIp;
    IPAddress _apNetmask;
    std::unique_ptr<DNSServer> _dnsServer;
    bool _dnsServerStatus = false;
    network_mode _networkMode = network_mode::Undefined;
    bool _ethConnected = false;
    std::vector<NetworkEventCbList_t> _cbEventList;
    bool _lastMdnsEnabled = false;
};

extern NetworkSettingsClass NetworkSettings;
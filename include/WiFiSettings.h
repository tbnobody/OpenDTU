#pragma once

#include <DNSServer.h>
#include <memory>

class WiFiSettingsClass {
public:
    WiFiSettingsClass();
    void init();
    void loop();
    void applyConfig();
    void enableAdminMode();
    String getApName();

private:
    void setHostname();
    void setStaticIp();
    void setupMode();
    bool adminEnabled = true;
    bool forceDisconnection = false;
    int adminTimeoutCounter = 0;
    int connectTimeoutTimer = 0;
    int connectRedoTimer = 0;
    unsigned long lastTimerCall = 0;
    const byte DNS_PORT = 53;
    IPAddress apIp;
    IPAddress apNetmask;
    std::unique_ptr<DNSServer> dnsServer;
};

extern WiFiSettingsClass WiFiSettings;
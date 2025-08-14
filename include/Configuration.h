// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <TaskSchedulerDeclarations.h>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011e00 // 0.1.30 // make sure to clean all after change

#define WIFI_MAX_SSID_STRLEN 32
#define WIFI_MAX_PASSWORD_STRLEN 64
#define WIFI_MAX_HOSTNAME_STRLEN 31

#define SYSLOG_MAX_HOSTNAME_STRLEN 128

#define NTP_MAX_SERVER_STRLEN 31
#define NTP_MAX_TIMEZONE_STRLEN 50
#define NTP_MAX_TIMEZONEDESCR_STRLEN 50

#define MQTT_MAX_HOSTNAME_STRLEN 128
#define MQTT_MAX_CLIENTID_STRLEN 64
#define MQTT_MAX_USERNAME_STRLEN 64
#define MQTT_MAX_PASSWORD_STRLEN 64
#define MQTT_MAX_TOPIC_STRLEN 32
#define MQTT_MAX_LWTVALUE_STRLEN 20
#define MQTT_MAX_CERT_STRLEN 2560

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 10
#define INV_MAX_CHAN_COUNT 6

#define CHAN_MAX_NAME_STRLEN 31

#define DEV_MAX_MAPPING_NAME_STRLEN 63
#define LOCALE_STRLEN 2

#define LOG_MODULE_COUNT 16
#define LOG_MODULE_NAME_STRLEN 32

struct CHANNEL_CONFIG_T {
    uint16_t MaxChannelPower;
    char Name[CHAN_MAX_NAME_STRLEN];
    float YieldTotalOffset;
};

struct INVERTER_CONFIG_T {
    uint64_t Serial;
    char Name[INV_MAX_NAME_STRLEN + 1];
    uint8_t Order;
    bool Poll_Enable;
    bool Poll_Enable_Night;
    bool Command_Enable;
    bool Command_Enable_Night;
    uint8_t ReachableThreshold;
    bool ZeroRuntimeDataIfUnrechable;
    bool ZeroYieldDayOnMidnight;
    bool ClearEventlogOnMidnight;
    bool YieldDayCorrection;
    CHANNEL_CONFIG_T channel[INV_MAX_CHAN_COUNT];
};

struct CONFIG_T {
    struct {
        uint32_t Version;
        uint32_t SaveCount;
    } Cfg;

    struct {
        char Ssid[WIFI_MAX_SSID_STRLEN + 1];
        char Password[WIFI_MAX_PASSWORD_STRLEN + 1];
        uint8_t Ip[4];
        uint8_t Netmask[4];
        uint8_t Gateway[4];
        uint8_t Dns1[4];
        uint8_t Dns2[4];
        bool Dhcp;
        char Hostname[WIFI_MAX_HOSTNAME_STRLEN + 1];
        uint32_t ApTimeout;
    } WiFi;

    struct {
        bool Enabled;
    } Mdns;

    struct {
        bool Enabled;
        char Hostname[SYSLOG_MAX_HOSTNAME_STRLEN + 1];
        uint16_t Port;
    } Syslog;

    struct {
        char Server[NTP_MAX_SERVER_STRLEN + 1];
        char Timezone[NTP_MAX_TIMEZONE_STRLEN + 1];
        char TimezoneDescr[NTP_MAX_TIMEZONEDESCR_STRLEN + 1];
        double Longitude;
        double Latitude;
        uint8_t SunsetType;
    } Ntp;

    struct {
        bool Enabled;
        char Hostname[MQTT_MAX_HOSTNAME_STRLEN + 1];
        uint32_t Port;
        char ClientId[MQTT_MAX_CLIENTID_STRLEN + 1];
        char Username[MQTT_MAX_USERNAME_STRLEN + 1];
        char Password[MQTT_MAX_PASSWORD_STRLEN + 1];
        char Topic[MQTT_MAX_TOPIC_STRLEN + 1];
        bool Retain;
        uint32_t PublishInterval;
        bool CleanSession;

        struct {
            char Topic[MQTT_MAX_TOPIC_STRLEN + 1];
            char Value_Online[MQTT_MAX_LWTVALUE_STRLEN + 1];
            char Value_Offline[MQTT_MAX_LWTVALUE_STRLEN + 1];
            uint8_t Qos;
        } Lwt;

        struct {
            bool Enabled;
            bool Retain;
            char Topic[MQTT_MAX_TOPIC_STRLEN + 1];
            bool IndividualPanels;
            bool Expire;
        } Hass;

        struct {
            bool Enabled;
            char RootCaCert[MQTT_MAX_CERT_STRLEN + 1];
            bool CertLogin;
            char ClientCert[MQTT_MAX_CERT_STRLEN + 1];
            char ClientKey[MQTT_MAX_CERT_STRLEN + 1];
        } Tls;
    } Mqtt;

    struct {
        uint64_t Serial;
        uint32_t PollInterval;
        struct {
            uint8_t PaLevel;
        } Nrf;
        struct {
            int8_t PaLevel;
            uint32_t Frequency;
            uint8_t CountryMode;
        } Cmt;
    } Dtu;

    struct {
        char Password[WIFI_MAX_PASSWORD_STRLEN + 1];
        bool AllowReadonly;
    } Security;

    struct {
        bool PowerSafe;
        bool ScreenSaver;
        uint8_t Rotation;
        uint8_t Contrast;
        char Locale[LOCALE_STRLEN + 1];
        struct {
            uint32_t Duration;
            uint8_t Mode;
        } Diagram;
    } Display;

    struct {
        uint8_t Brightness;
    } Led_Single[PINMAPPING_LED_COUNT];

    INVERTER_CONFIG_T Inverter[INV_MAX_COUNT];
    char Dev_PinMapping[DEV_MAX_MAPPING_NAME_STRLEN + 1];

    struct {
        int8_t Default;
        struct {
            char Name[LOG_MODULE_NAME_STRLEN + 1];
            int8_t Level;
        } Modules[LOG_MODULE_COUNT];
    } Logging;
};

class ConfigurationClass {
public:
    void init(Scheduler& scheduler);
    bool read();
    bool write();
    void migrate();
    CONFIG_T const& get();

    class WriteGuard {
    public:
        WriteGuard();
        CONFIG_T& getConfig();
        ~WriteGuard();

    private:
        std::unique_lock<std::mutex> _lock;
    };

    WriteGuard getWriteGuard();

    INVERTER_CONFIG_T* getFreeInverterSlot();
    INVERTER_CONFIG_T* getInverterConfig(const uint64_t serial);
    void deleteInverterById(const uint8_t id);

    int8_t getIndexForLogModule(const String& moduleName) const;

private:
    void loop();

    Task _loopTask;
};

extern ConfigurationClass Configuration;

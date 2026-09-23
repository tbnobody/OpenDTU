// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011e00 // 0.1.30 // make sure to clean all after change

#define WIFI_MAX_SSID_STRLEN 32
#define WIFI_MAX_PASSWORD_STRLEN 64
#define WIFI_MAX_HOSTNAME_STRLEN 31

#define SYSLOG_MAX_HOSTNAME_STRLEN 128

#define NTP_MAX_SERVER_STRLEN 31
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
    void init();
    bool read();
    bool write();
    void migrate();
    CONFIG_T const& get();

    // Runs fn with exclusive write access to the configuration, holding the
    // config mutex for the duration of the call. Safe to call from any task,
    // including the AsyncTCP task that runs the web API callbacks.
    //
    // The return type is deduced from the callable: a lambda returning a
    // value forwards it to the caller, a lambda returning void simply
    // mutates. Plain lambdas work (same deduction pattern as read(Fn)).
    //
    // Usage (mutation):
    //   Configuration.update([](CONFIG_T& cfg) {
    //       cfg.foo = bar;
    //   });
    //
    // Usage (mutation with result):
    //   auto* slot = Configuration.update([](CONFIG_T& cfg) {
    //       return getFreeInverterSlot(cfg);
    //   });
    template <typename Fn>
    auto update(Fn&& fn) -> decltype(fn(std::declval<CONFIG_T&>()))
    {
        std::unique_lock<std::shared_mutex> lock(sConfigMutex);
        return fn(config);
    }

    // Consistent read: runs fn on the config while holding the shared lock,
    // so the values fn reads can never be torn by a concurrent update().
    // Multiple readers can run at the same time; writes stay exclusive.
    // fn must only read (not mutate) the config and must not block; use it
    // when several related fields must be captured atomically. For single
    // scalar fields, the lock-free Configuration.get() is fine.
    //
    // Usage:
    //   auto pos = Configuration.read([](CONFIG_T const& cfg) {
    //       return std::make_pair(cfg.Ntp.Latitude, cfg.Ntp.Longitude);
    //   });
    template <typename Fn>
    auto read(Fn&& fn) const -> decltype(fn(std::declval<CONFIG_T const&>()))
    {
        std::shared_lock<std::shared_mutex> lock(sConfigMutex);
        return fn(config);
    }

    // Read helpers operating on the global configuration. Kept as-is for now;
    // reads are intentionally not locked.
    INVERTER_CONFIG_T* getInverterConfig(const uint64_t serial);
    int8_t getIndexForLogModule(const String& moduleName) const;

    // Write helpers, meant to be used within update() on the passed config.
    static INVERTER_CONFIG_T* getFreeInverterSlot(CONFIG_T& config);
    static void deleteInverterById(CONFIG_T& config, const uint8_t id);

private:
    // The in-RAM configuration and the read-write mutex protecting it.
    // Config is written from the AsyncTCP task (web API callbacks) and read
    // from the main task (scheduler loop) as well as from AsyncTCP. Readers
    // take a shared lock (read(Fn)), writers a unique lock (update(),
    // read()/write()/migrate()) — multiple readers can run concurrently,
    // writes are exclusive.
    // NOT recursive: never call update()/read(Fn)/read()/write()
    // from inside another update() lambda — that would deadlock on the
    // lock this thread already holds.
    static CONFIG_T config;
    static std::shared_mutex sConfigMutex;

    // Flash I/O helpers; call only while sConfigMutex is held.
    static bool writeConfigLocked();
    static bool readConfigLocked();
};

extern ConfigurationClass Configuration;

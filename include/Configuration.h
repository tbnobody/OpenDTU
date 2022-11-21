// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011700 // 0.1.23 // make sure to clean all after change

#define WIFI_MAX_SSID_STRLEN 31
#define WIFI_MAX_PASSWORD_STRLEN 64
#define WIFI_MAX_HOSTNAME_STRLEN 31

#define NTP_MAX_SERVER_STRLEN 31
#define NTP_MAX_TIMEZONE_STRLEN 50
#define NTP_MAX_TIMEZONEDESCR_STRLEN 50

#define MQTT_MAX_HOSTNAME_STRLEN 128
#define MQTT_MAX_USERNAME_STRLEN 64
#define MQTT_MAX_PASSWORD_STRLEN 64
#define MQTT_MAX_TOPIC_STRLEN 32
#define MQTT_MAX_LWTVALUE_STRLEN 20
#define MQTT_MAX_ROOT_CA_CERT_STRLEN 2048

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 10
#define INV_MAX_CHAN_COUNT 4

#define CHAN_MAX_NAME_STRLEN 31

#define JSON_BUFFER_SIZE 6144

struct CHANNEL_CONFIG_T {
    uint16_t MaxChannelPower;
    char Name[CHAN_MAX_NAME_STRLEN];
};

struct INVERTER_CONFIG_T {
    uint64_t Serial;
    char Name[INV_MAX_NAME_STRLEN + 1];
    uint16_t CurrentPhase;
    CHANNEL_CONFIG_T channel[INV_MAX_CHAN_COUNT];
};

struct CONFIG_T {
    uint32_t Cfg_Version;
    uint Cfg_SaveCount;

    char WiFi_Ssid[WIFI_MAX_SSID_STRLEN + 1];
    char WiFi_Password[WIFI_MAX_PASSWORD_STRLEN + 1];
    byte WiFi_Ip[4];
    byte WiFi_Netmask[4];
    byte WiFi_Gateway[4];
    byte WiFi_Dns1[4];
    byte WiFi_Dns2[4];
    bool WiFi_Dhcp;
    char WiFi_Hostname[WIFI_MAX_HOSTNAME_STRLEN + 1];

    char Ntp_Server[NTP_MAX_SERVER_STRLEN + 1];
    char Ntp_Timezone[NTP_MAX_TIMEZONE_STRLEN + 1];
    char Ntp_TimezoneDescr[NTP_MAX_TIMEZONEDESCR_STRLEN + 1];

    bool Mqtt_Enabled;
    uint Mqtt_Port;
    char Mqtt_Username[MQTT_MAX_USERNAME_STRLEN + 1];
    char Mqtt_Password[MQTT_MAX_PASSWORD_STRLEN + 1];
    char Mqtt_Topic[MQTT_MAX_TOPIC_STRLEN + 1];
    bool Mqtt_Retain;
    char Mqtt_LwtTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char Mqtt_LwtValue_Online[MQTT_MAX_LWTVALUE_STRLEN + 1];
    char Mqtt_LwtValue_Offline[MQTT_MAX_LWTVALUE_STRLEN + 1];
    uint32_t Mqtt_PublishInterval;

    INVERTER_CONFIG_T Inverter[INV_MAX_COUNT];

    uint64_t Dtu_Serial;
    uint32_t Dtu_PollInterval;
    uint8_t Dtu_PaLevel;

    bool Mqtt_Hass_Enabled;
    bool Mqtt_Hass_Retain;
    char Mqtt_Hass_Topic[MQTT_MAX_TOPIC_STRLEN + 1];
    bool Mqtt_Hass_IndividualPanels;
    bool Mqtt_Tls;
    char Mqtt_RootCaCert[MQTT_MAX_ROOT_CA_CERT_STRLEN + 1];

    char Mqtt_Hostname[MQTT_MAX_HOSTNAME_STRLEN + 1];

    bool Mqtt_Hass_Expire;

    char Security_Password[WIFI_MAX_PASSWORD_STRLEN + 1];

    bool Mqtt_Victron_Enabled;
};

class ConfigurationClass {
public:
    void init();
    bool read();
    bool write();
    void migrate();
    CONFIG_T& get();

    INVERTER_CONFIG_T* getFreeInverterSlot();
    INVERTER_CONFIG_T* getInverterConfig(uint64_t serial);
};

extern ConfigurationClass Configuration;
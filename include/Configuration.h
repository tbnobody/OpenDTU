#pragma once

#include <Arduino.h>

#define CONFIG_FILENAME "/config.bin"
#define CONFIG_VERSION 0x00010800 // 0.1.8 // make sure to clean all after change

#define WIFI_MAX_SSID_STRLEN 31
#define WIFI_MAX_PASSWORD_STRLEN 31
#define WIFI_MAX_HOSTNAME_STRLEN 31

#define NTP_MAX_SERVER_STRLEN 31
#define NTP_MAX_TIMEZONE_STRLEN 50
#define NTP_MAX_TIMEZONEDESCR_STRLEN 50

#define MQTT_MAX_HOSTNAME_STRLEN 31
#define MQTT_MAX_USERNAME_STRLEN 32
#define MQTT_MAX_PASSWORD_STRLEN 32
#define MQTT_MAX_TOPIC_STRLEN 32
#define MQTT_MAX_LWTVALUE_STRLEN 20

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 10

struct INVERTER_CONFIG_T {
    uint64_t Serial;
    char Name[INV_MAX_NAME_STRLEN + 1];
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
    char Mqtt_Hostname[MQTT_MAX_HOSTNAME_STRLEN + 1];
    uint Mqtt_Port;
    char Mqtt_Username[MQTT_MAX_USERNAME_STRLEN + 1];
    char Mqtt_Password[MQTT_MAX_PASSWORD_STRLEN + 1];
    char Mqtt_Topic[MQTT_MAX_TOPIC_STRLEN + 1];
    bool Mqtt_Retain;
    char Mqtt_LwtTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char Mqtt_LwtValue_Online[MQTT_MAX_LWTVALUE_STRLEN + 1];
    char Mqtt_LwtValue_Offline[MQTT_MAX_LWTVALUE_STRLEN + 1];

    INVERTER_CONFIG_T Inverter[INV_MAX_COUNT];
};

class ConfigurationClass {
public:
    void init();
    bool read();
    bool write();
    void migrate();
    CONFIG_T& get();

    INVERTER_CONFIG_T* getFreeInverterSlot();
};

extern ConfigurationClass Configuration;
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <cstdint>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011a00 // 0.1.26 // make sure to clean all after change

#define WIFI_MAX_SSID_STRLEN 32
#define WIFI_MAX_PASSWORD_STRLEN 64
#define WIFI_MAX_HOSTNAME_STRLEN 31

#define NTP_MAX_SERVER_STRLEN 31
#define NTP_MAX_TIMEZONE_STRLEN 50
#define NTP_MAX_TIMEZONEDESCR_STRLEN 50

#define MQTT_MAX_HOSTNAME_STRLEN 128
#define MQTT_MAX_USERNAME_STRLEN 64
#define MQTT_MAX_PASSWORD_STRLEN 64
#define MQTT_MAX_TOPIC_STRLEN 256
#define MQTT_MAX_LWTVALUE_STRLEN 20
#define MQTT_MAX_CERT_STRLEN 2560

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 5
#define INV_MAX_CHAN_COUNT 6

#define CHAN_MAX_NAME_STRLEN 31

#define DEV_MAX_MAPPING_NAME_STRLEN 63

#define POWERMETER_MAX_PHASES 3
#define POWERMETER_MAX_HTTP_URL_STRLEN 1024
#define POWERMETER_MAX_USERNAME_STRLEN 64
#define POWERMETER_MAX_PASSWORD_STRLEN 64
#define POWERMETER_MAX_HTTP_HEADER_KEY_STRLEN 64
#define POWERMETER_MAX_HTTP_HEADER_VALUE_STRLEN 256
#define POWERMETER_MAX_HTTP_JSON_PATH_STRLEN 256
#define POWERMETER_HTTP_TIMEOUT 1000

#define JSON_BUFFER_SIZE 15360

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
    bool YieldDayCorrection;
    CHANNEL_CONFIG_T channel[INV_MAX_CHAN_COUNT];
};

enum Auth { none, basic, digest };
struct POWERMETER_HTTP_PHASE_CONFIG_T {
    bool Enabled;
    char Url[POWERMETER_MAX_HTTP_URL_STRLEN + 1];
    Auth AuthType;
    char Username[POWERMETER_MAX_USERNAME_STRLEN +1];
    char Password[POWERMETER_MAX_USERNAME_STRLEN +1];
    char HeaderKey[POWERMETER_MAX_HTTP_HEADER_KEY_STRLEN + 1];
    char HeaderValue[POWERMETER_MAX_HTTP_HEADER_VALUE_STRLEN + 1];
    uint16_t Timeout;
    char JsonPath[POWERMETER_MAX_HTTP_JSON_PATH_STRLEN + 1];
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
        bool VerboseLogging;
        uint32_t Port;
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
        } Cmt;
        bool VerboseLogging;
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
        uint8_t Language;
        uint32_t DiagramDuration;
    } Display;

    struct {
        uint8_t Brightness;
    } Led_Single[PINMAPPING_LED_COUNT];

    struct {
        bool Enabled;
        bool VerboseLogging;
        bool UpdatesOnly;
    } Vedirect;

    struct {
        bool Enabled;
        bool VerboseLogging;
        uint32_t Interval;
        uint32_t Source;
        char MqttTopicPowerMeter1[MQTT_MAX_TOPIC_STRLEN + 1];
        char MqttTopicPowerMeter2[MQTT_MAX_TOPIC_STRLEN + 1];
        char MqttTopicPowerMeter3[MQTT_MAX_TOPIC_STRLEN + 1];
        uint32_t SdmBaudrate;
        uint32_t SdmAddress;
        uint32_t HttpInterval;
        bool HttpIndividualRequests;
        POWERMETER_HTTP_PHASE_CONFIG_T Http_Phase[POWERMETER_MAX_PHASES];
    } PowerMeter;
    
    struct {
        bool Enabled;
        bool VerboseLogging;
        bool SolarPassThroughEnabled;
        uint8_t SolarPassThroughLosses;
        uint8_t BatteryDrainStategy;
        uint32_t Interval;
        bool IsInverterBehindPowerMeter;
        uint8_t InverterId;
        uint8_t InverterChannelId;
        int32_t TargetPowerConsumption;
        int32_t TargetPowerConsumptionHysteresis;
        int32_t LowerPowerLimit;
        int32_t UpperPowerLimit;
        uint32_t BatterySocStartThreshold;
        uint32_t BatterySocStopThreshold;
        float VoltageStartThreshold;
        float VoltageStopThreshold;
        float VoltageLoadCorrectionFactor;
        int8_t RestartHour;
        uint32_t FullSolarPassThroughSoc;
        float FullSolarPassThroughStartVoltage;
        float FullSolarPassThroughStopVoltage;
    } PowerLimiter;
    
    struct {
        bool Enabled;
        bool VerboseLogging;
        uint8_t Provider;
        uint8_t JkBmsInterface;
        uint8_t JkBmsPollingInterval;
        char MqttTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    } Battery;

    struct {
        bool Enabled;
        uint32_t CAN_Controller_Frequency;
        bool Auto_Power_Enabled;
        float Auto_Power_Voltage_Limit;
        float Auto_Power_Enable_Voltage_Limit;
        float Auto_Power_Lower_Power_Limit;
        float Auto_Power_Upper_Power_Limit;   
    } Huawei;


    INVERTER_CONFIG_T Inverter[INV_MAX_COUNT];
    char Dev_PinMapping[DEV_MAX_MAPPING_NAME_STRLEN + 1];
};

class ConfigurationClass {
public:
    void init();
    bool read();
    bool write();
    void migrate();
    CONFIG_T& get();

    INVERTER_CONFIG_T* getFreeInverterSlot();
    INVERTER_CONFIG_T* getInverterConfig(const uint64_t serial);
};

extern ConfigurationClass Configuration;
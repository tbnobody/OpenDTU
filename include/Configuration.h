// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <cstdint>
#include <ArduinoJson.h>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011c00 // 0.1.28 // make sure to clean all after change

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
#define MQTT_MAX_TOPIC_STRLEN 256
#define MQTT_MAX_LWTVALUE_STRLEN 20
#define MQTT_MAX_CERT_STRLEN 2560

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 10
#define INV_MAX_CHAN_COUNT 6

#define CHAN_MAX_NAME_STRLEN 31

#define DEV_MAX_MAPPING_NAME_STRLEN 63

#define HTTP_REQUEST_MAX_URL_STRLEN 1024
#define HTTP_REQUEST_MAX_USERNAME_STRLEN 64
#define HTTP_REQUEST_MAX_PASSWORD_STRLEN 64
#define HTTP_REQUEST_MAX_HEADER_KEY_STRLEN 64
#define HTTP_REQUEST_MAX_HEADER_VALUE_STRLEN 256

#define POWERMETER_MQTT_MAX_VALUES 3
#define POWERMETER_HTTP_JSON_MAX_VALUES 3
#define POWERMETER_HTTP_JSON_MAX_PATH_STRLEN 256
#define BATTERY_JSON_MAX_PATH_STRLEN 128

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

struct HTTP_REQUEST_CONFIG_T {
    char Url[HTTP_REQUEST_MAX_URL_STRLEN + 1];

    enum Auth { None, Basic, Digest };
    Auth AuthType;

    char Username[HTTP_REQUEST_MAX_USERNAME_STRLEN + 1];
    char Password[HTTP_REQUEST_MAX_PASSWORD_STRLEN + 1];
    char HeaderKey[HTTP_REQUEST_MAX_HEADER_KEY_STRLEN + 1];
    char HeaderValue[HTTP_REQUEST_MAX_HEADER_VALUE_STRLEN + 1];
    uint16_t Timeout;
};
using HttpRequestConfig = struct HTTP_REQUEST_CONFIG_T;

struct POWERMETER_MQTT_VALUE_T {
    char Topic[MQTT_MAX_TOPIC_STRLEN + 1];
    char JsonPath[POWERMETER_HTTP_JSON_MAX_PATH_STRLEN + 1];

    enum Unit { Watts = 0, MilliWatts = 1, KiloWatts = 2 };
    Unit PowerUnit;

    bool SignInverted;
};
using PowerMeterMqttValue = struct POWERMETER_MQTT_VALUE_T;

struct POWERMETER_MQTT_CONFIG_T {
    PowerMeterMqttValue Values[POWERMETER_MQTT_MAX_VALUES];
};
using PowerMeterMqttConfig = struct POWERMETER_MQTT_CONFIG_T;

struct POWERMETER_SERIAL_SDM_CONFIG_T {
    uint32_t Address;
    uint32_t PollingInterval;
};
using PowerMeterSerialSdmConfig = struct POWERMETER_SERIAL_SDM_CONFIG_T;

struct POWERMETER_HTTP_JSON_VALUE_T {
    HttpRequestConfig HttpRequest;
    bool Enabled;
    char JsonPath[POWERMETER_HTTP_JSON_MAX_PATH_STRLEN + 1];

    enum Unit { Watts = 0, MilliWatts = 1, KiloWatts = 2 };
    Unit PowerUnit;

    bool SignInverted;
};
using PowerMeterHttpJsonValue = struct POWERMETER_HTTP_JSON_VALUE_T;

struct POWERMETER_HTTP_JSON_CONFIG_T {
    uint32_t PollingInterval;
    bool IndividualRequests;
    PowerMeterHttpJsonValue Values[POWERMETER_HTTP_JSON_MAX_VALUES];
};
using PowerMeterHttpJsonConfig = struct POWERMETER_HTTP_JSON_CONFIG_T;

struct POWERMETER_HTTP_SML_CONFIG_T {
    uint32_t PollingInterval;
    HttpRequestConfig HttpRequest;
};
using PowerMeterHttpSmlConfig = struct POWERMETER_HTTP_SML_CONFIG_T;

enum BatteryVoltageUnit { Volts = 0, DeciVolts = 1, CentiVolts = 2, MilliVolts = 3 };

enum BatteryAmperageUnit { Amps = 0, MilliAmps = 1 };

struct BATTERY_CONFIG_T {
    bool Enabled;
    bool VerboseLogging;
    uint8_t Provider;
    uint8_t JkBmsInterface;
    uint8_t JkBmsPollingInterval;
    char MqttSocTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char MqttSocJsonPath[BATTERY_JSON_MAX_PATH_STRLEN + 1];
    char MqttVoltageTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char MqttVoltageJsonPath[BATTERY_JSON_MAX_PATH_STRLEN + 1];
    BatteryVoltageUnit MqttVoltageUnit;
    bool EnableDischargeCurrentLimit;
    float DischargeCurrentLimit;
    float DischargeCurrentLimitBelowSoc;
    float DischargeCurrentLimitBelowVoltage;
    bool UseBatteryReportedDischargeCurrentLimit;
    char MqttDischargeCurrentTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char MqttDischargeCurrentJsonPath[BATTERY_JSON_MAX_PATH_STRLEN + 1];
    BatteryAmperageUnit MqttAmperageUnit;
};
using BatteryConfig = struct BATTERY_CONFIG_T;

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
        bool VerboseLogging;
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
        struct {
            uint32_t Duration;
            uint8_t Mode;
        } Diagram;
    } Display;

    struct {
        uint8_t Brightness;
    } Led_Single[PINMAPPING_LED_COUNT];

    struct {
        bool Enabled;
        bool VerboseLogging;
        bool UpdatesOnly;
    } Vedirect;

    struct PowerMeterConfig {
        bool Enabled;
        bool VerboseLogging;
        uint32_t Source;
        PowerMeterMqttConfig Mqtt;
        PowerMeterSerialSdmConfig SerialSdm;
        PowerMeterHttpJsonConfig HttpJson;
        PowerMeterHttpSmlConfig HttpSml;
    } PowerMeter;

    struct {
        bool Enabled;
        bool VerboseLogging;
        bool SolarPassThroughEnabled;
        uint8_t SolarPassThroughLosses;
        bool BatteryAlwaysUseAtNight;
        uint32_t Interval;
        bool IsInverterBehindPowerMeter;
        bool IsInverterSolarPowered;
        bool UseOverscalingToCompensateShading;
        uint64_t InverterId;
        uint8_t InverterChannelId;
        int32_t TargetPowerConsumption;
        int32_t TargetPowerConsumptionHysteresis;
        int32_t LowerPowerLimit;
        int32_t BaseLoadLimit;
        int32_t UpperPowerLimit;
        bool IgnoreSoc;
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

    BatteryConfig Battery;

    struct {
        bool Enabled;
        bool VerboseLogging;
        uint32_t CAN_Controller_Frequency;
        bool Auto_Power_Enabled;
        bool Auto_Power_BatterySoC_Limits_Enabled;
        bool Emergency_Charge_Enabled;
        float Auto_Power_Voltage_Limit;
        float Auto_Power_Enable_Voltage_Limit;
        float Auto_Power_Lower_Power_Limit;
        float Auto_Power_Upper_Power_Limit;
        uint8_t Auto_Power_Stop_BatterySoC_Threshold;
        float Auto_Power_Target_Power_Consumption;
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
    void deleteInverterById(const uint8_t id);

    static void serializeHttpRequestConfig(HttpRequestConfig const& source, JsonObject& target);
    static void serializePowerMeterMqttConfig(PowerMeterMqttConfig const& source, JsonObject& target);
    static void serializePowerMeterSerialSdmConfig(PowerMeterSerialSdmConfig const& source, JsonObject& target);
    static void serializePowerMeterHttpJsonConfig(PowerMeterHttpJsonConfig const& source, JsonObject& target);
    static void serializePowerMeterHttpSmlConfig(PowerMeterHttpSmlConfig const& source, JsonObject& target);
    static void serializeBatteryConfig(BatteryConfig const& source, JsonObject& target);

    static void deserializeHttpRequestConfig(JsonObject const& source, HttpRequestConfig& target);
    static void deserializePowerMeterMqttConfig(JsonObject const& source, PowerMeterMqttConfig& target);
    static void deserializePowerMeterSerialSdmConfig(JsonObject const& source, PowerMeterSerialSdmConfig& target);
    static void deserializePowerMeterHttpJsonConfig(JsonObject const& source, PowerMeterHttpJsonConfig& target);
    static void deserializePowerMeterHttpSmlConfig(JsonObject const& source, PowerMeterHttpSmlConfig& target);
    static void deserializeBatteryConfig(JsonObject const& source, BatteryConfig& target);
};

extern ConfigurationClass Configuration;

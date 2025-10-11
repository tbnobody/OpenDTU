// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PinMapping.h"
#include <TaskSchedulerDeclarations.h>
#include <condition_variable>
#include <cstdint>
#include <ArduinoJson.h>
#include <mutex>

#define CONFIG_FILENAME "/config.json"
#define CONFIG_VERSION 0x00011e00 // 0.1.30 // make sure to clean all after change
#define CONFIG_VERSION_ONBATTERY 8

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
#define MQTT_MAX_JSON_PATH_STRLEN 256

#define INV_MAX_NAME_STRLEN 31
#define INV_MAX_COUNT 10
#define INV_MAX_CHAN_COUNT 6

#define CHAN_MAX_NAME_STRLEN 31

#define DEV_MAX_MAPPING_NAME_STRLEN 63
#define LOCALE_STRLEN 2

#define LOG_MODULE_COUNT 16
#define LOG_MODULE_NAME_STRLEN 32

#define HTTP_REQUEST_MAX_URL_STRLEN 1024
#define HTTP_REQUEST_MAX_USERNAME_STRLEN 64
#define HTTP_REQUEST_MAX_PASSWORD_STRLEN 64
#define HTTP_REQUEST_MAX_HEADER_KEY_STRLEN 64
#define HTTP_REQUEST_MAX_HEADER_VALUE_STRLEN 256

#define POWERMETER_MQTT_MAX_VALUES 3
#define POWERMETER_HTTP_JSON_MAX_VALUES 3

#define ZENDURE_MAX_SERIAL_STRLEN 8

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
    char JsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];

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
    char JsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];

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

struct POWERMETER_UDP_VICTRON_CONFIG_T {
    uint16_t PollingIntervalMs;
    uint8_t IpAddress[4];
};
using PowerMeterUdpVictronConfig = struct POWERMETER_UDP_VICTRON_CONFIG_T;

struct POWERLIMITER_INVERTER_CONFIG_T {
    uint64_t Serial;
    bool IsGoverned;
    bool IsBehindPowerMeter;
    bool UseOverscaling;
    bool AllowStandby;
    uint16_t LowerPowerLimit;
    uint16_t UpperPowerLimit;

    enum InverterPowerSource { Battery = 0, Solar = 1, SmartBuffer = 2 };
    InverterPowerSource PowerSource;
};
using PowerLimiterInverterConfig = struct POWERLIMITER_INVERTER_CONFIG_T;

struct POWERLIMITER_CONFIG_T {
    bool Enabled;
    bool SolarPassThroughEnabled;
    uint8_t ConductionLosses;
    bool BatteryAlwaysUseAtNight;
    int16_t TargetPowerConsumption;
    uint16_t TargetPowerConsumptionHysteresis;
    uint16_t BaseLoadLimit;
    bool IgnoreSoc;
    uint16_t BatterySocStartThreshold;
    uint16_t BatterySocStopThreshold;
    float VoltageStartThreshold;
    float VoltageStopThreshold;
    float VoltageLoadCorrectionFactor;
    uint16_t FullSolarPassThroughSoc;
    float FullSolarPassThroughStartVoltage;
    float FullSolarPassThroughStopVoltage;
    uint64_t InverterSerialForDcVoltage;
    uint8_t InverterChannelIdForDcVoltage;
    uint8_t RestartHour;
    uint16_t TotalUpperPowerLimit;
    PowerLimiterInverterConfig Inverters[INV_MAX_COUNT];
};
using PowerLimiterConfig = struct POWERLIMITER_CONFIG_T;

struct BATTERY_ZENDURE_CONFIG_T {
    uint8_t DeviceType;
    char DeviceId[ZENDURE_MAX_SERIAL_STRLEN + 1];
    uint8_t PollingInterval;
    uint8_t MinSoC;
    uint8_t MaxSoC;
    uint8_t BypassMode;
    uint16_t MaxOutput;
    bool AutoShutdown;
    uint16_t OutputLimit;
    enum ZendureBatteryOutputControl { ControlNone = 0, ControlFixed = 1, ControlSchedule = 2 };
    ZendureBatteryOutputControl OutputControl;
    int16_t SunriseOffset;
    int16_t SunsetOffset;
    uint16_t OutputLimitDay;
    uint16_t OutputLimitNight;
    bool ChargeThroughEnable;
    uint16_t ChargeThroughInterval;
    bool BuzzerEnable;
    enum ControlMode { ControlModeFull = 0, ControlModeOnce = 1, ControlModeReadOnly = 2 };
    ControlMode ControlMode;
    uint8_t ChargeThroughResetLevel;
};
using BatteryZendureConfig = struct BATTERY_ZENDURE_CONFIG_T;

enum BatteryVoltageUnit { Volts = 0, DeciVolts = 1, CentiVolts = 2, MilliVolts = 3 };
enum BatteryAmperageUnit { Amps = 0, MilliAmps = 1 };
struct BATTERY_MQTT_CONFIG_T {
    char SocTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char SocJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    char VoltageTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char VoltageJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    BatteryVoltageUnit VoltageUnit;
    char CurrentTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char CurrentJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    BatteryAmperageUnit CurrentUnit;
    char DischargeCurrentLimitTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char DischargeCurrentLimitJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    BatteryAmperageUnit DischargeCurrentLimitUnit;
};
using BatteryMqttConfig = struct BATTERY_MQTT_CONFIG_T;

struct BATTERY_SERIAL_CONFIG_T {
    uint8_t Interface;
    uint8_t PollingInterval;
};
using BatterySerialConfig = struct BATTERY_SERIAL_CONFIG_T;

struct BATTERY_CONFIG_T {
    bool Enabled;
    uint8_t Provider;
    BatteryMqttConfig Mqtt;
    BatteryZendureConfig Zendure;
    BatterySerialConfig Serial;
    bool EnableDischargeCurrentLimit;
    float DischargeCurrentLimit;
    float DischargeCurrentLimitBelowSoc;
    float DischargeCurrentLimitBelowVoltage;
    bool UseBatteryReportedDischargeCurrentLimit;
};
using BatteryConfig = struct BATTERY_CONFIG_T;

enum GridChargerProviderType { HUAWEI = 0, TRUCKI = 1 };
enum GridChargerHardwareInterface { MCP2515 = 0, TWAI = 1 };

struct GRID_CHARGER_TRUCKI_CONFIG_T {
    uint8_t IpAddress[4];
    char Password[HTTP_REQUEST_MAX_PASSWORD_STRLEN + 1];
};
using GridChargerTruckiConfig = struct GRID_CHARGER_TRUCKI_CONFIG_T;

struct GRID_CHARGER_CAN_CONFIG_T {
    GridChargerHardwareInterface HardwareInterface;
    uint32_t Controller_Frequency;
};
using GridChargerCanConfig = struct GRID_CHARGER_CAN_CONFIG_T;

struct GRID_CHARGER_HUAWEI_CONFIG_T {
    float OfflineVoltage;
    float OfflineCurrent;
    float InputCurrentLimit;
    bool FanOnlineFullSpeed;
    bool FanOfflineFullSpeed;
};
using GridChargerHuaweiConfig = struct GRID_CHARGER_HUAWEI_CONFIG_T;

struct GRID_CHARGER_CONFIG_T {
    bool Enabled;
    bool AutoPowerEnabled;
    bool AutoPowerBatterySoCLimitsEnabled;
    bool EmergencyChargeEnabled;
    float AutoPowerVoltageLimit;
    float AutoPowerEnableVoltageLimit;
    float AutoPowerLowerPowerLimit;
    float AutoPowerUpperPowerLimit;
    uint8_t AutoPowerStopBatterySoCThreshold;
    float AutoPowerTargetPowerConsumption;
    GridChargerProviderType Provider;
    GridChargerCanConfig Can;
    GridChargerHuaweiConfig Huawei;
    GridChargerTruckiConfig Trucki;
};
using GridChargerConfig = struct GRID_CHARGER_CONFIG_T;

enum SolarChargerProviderType { VEDIRECT = 0, MQTT = 1 };

struct SOLARCHARGER_MQTT_CONFIG_T {
    bool CalculateOutputPower;

    enum WattageUnit { KiloWatts = 0, Watts = 1, MilliWatts = 2 };
    char PowerTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char PowerJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    WattageUnit PowerUnit;

    enum VoltageUnit { Volts = 0, DeciVolts = 1, CentiVolts = 2, MilliVolts = 3 };
    char VoltageTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char VoltageJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    VoltageUnit VoltageTopicUnit;

    enum AmperageUnit { Amps = 0, MilliAmps = 1 };
    char CurrentTopic[MQTT_MAX_TOPIC_STRLEN + 1];
    char CurrentJsonPath[MQTT_MAX_JSON_PATH_STRLEN + 1];
    AmperageUnit CurrentUnit;
};
using SolarChargerMqttConfig = struct SOLARCHARGER_MQTT_CONFIG_T;

struct SOLAR_CHARGER_CONFIG_T {
    bool Enabled;
    bool PublishUpdatesOnly;
    SolarChargerProviderType Provider;
    SolarChargerMqttConfig Mqtt;
};
using SolarChargerConfig = struct SOLAR_CHARGER_CONFIG_T;

struct CONFIG_T {
    struct {
        uint32_t Version;
        uint32_t VersionOnBattery;
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

    SolarChargerConfig SolarCharger;

    struct PowerMeterConfig {
        bool Enabled;
        uint32_t Source;
        PowerMeterMqttConfig Mqtt;
        PowerMeterSerialSdmConfig SerialSdm;
        PowerMeterHttpJsonConfig HttpJson;
        PowerMeterHttpSmlConfig HttpSml;
        PowerMeterUdpVictronConfig UdpVictron;
    } PowerMeter;

    PowerLimiterConfig PowerLimiter;

    BatteryConfig Battery;

    GridChargerConfig GridCharger;

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
    void migrateOnBattery();
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

    static void serializeHttpRequestConfig(HttpRequestConfig const& source, JsonObject& target);
    static void serializeSolarChargerConfig(SolarChargerConfig const& source, JsonObject& target);
    static void serializeSolarChargerMqttConfig(SolarChargerMqttConfig const& source, JsonObject& target);
    static void serializePowerMeterMqttConfig(PowerMeterMqttConfig const& source, JsonObject& target);
    static void serializePowerMeterSerialSdmConfig(PowerMeterSerialSdmConfig const& source, JsonObject& target);
    static void serializePowerMeterHttpJsonConfig(PowerMeterHttpJsonConfig const& source, JsonObject& target);
    static void serializePowerMeterHttpSmlConfig(PowerMeterHttpSmlConfig const& source, JsonObject& target);
    static void serializePowerMeterUdpVictronConfig(PowerMeterUdpVictronConfig const& source, JsonObject& target);
    static void serializeBatteryConfig(BatteryConfig const& source, JsonObject& target);
    static void serializeBatteryZendureConfig(BatteryZendureConfig const& source, JsonObject& target);
    static void serializeBatteryMqttConfig(BatteryMqttConfig const& source, JsonObject& target);
    static void serializeBatterySerialConfig(BatterySerialConfig const& source, JsonObject& target);
    static void serializePowerLimiterConfig(PowerLimiterConfig const& source, JsonObject& target);
    static void serializeGridChargerConfig(GridChargerConfig const& source, JsonObject& target);
    static void serializeGridChargerCanConfig(GridChargerCanConfig const& source, JsonObject& target);
    static void serializeGridChargerHuaweiConfig(GridChargerHuaweiConfig const& source, JsonObject& target);
    static void serializeGridChargerTruckiConfig(GridChargerTruckiConfig const& source, JsonObject& target);

    static void deserializeHttpRequestConfig(JsonObject const& source_http_config, HttpRequestConfig& target);
    static void deserializeSolarChargerConfig(JsonObject const& source, SolarChargerConfig& target);
    static void deserializeSolarChargerMqttConfig(JsonObject const& source, SolarChargerMqttConfig& target);
    static void deserializePowerMeterMqttConfig(JsonObject const& source, PowerMeterMqttConfig& target);
    static void deserializePowerMeterSerialSdmConfig(JsonObject const& source, PowerMeterSerialSdmConfig& target);
    static void deserializePowerMeterHttpJsonConfig(JsonObject const& source, PowerMeterHttpJsonConfig& target);
    static void deserializePowerMeterHttpSmlConfig(JsonObject const& source, PowerMeterHttpSmlConfig& target);
    static void deserializePowerMeterUdpVictronConfig(JsonObject const& source, PowerMeterUdpVictronConfig& target);
    static void deserializeBatteryConfig(JsonObject const& source, BatteryConfig& target);
    static void deserializeBatteryZendureConfig(JsonObject const& source, BatteryZendureConfig& target);
    static void deserializeBatteryMqttConfig(JsonObject const& source, BatteryMqttConfig& target);
    static void deserializeBatterySerialConfig(JsonObject const& source, BatterySerialConfig& target);
    static void deserializePowerLimiterConfig(JsonObject const& source, PowerLimiterConfig& target);
    static void deserializeGridChargerConfig(JsonObject const& source, GridChargerConfig& target);
    static void deserializeGridChargerCanConfig(JsonObject const& source, GridChargerCanConfig& target);
    static void deserializeGridChargerHuaweiConfig(JsonObject const& source, GridChargerHuaweiConfig& target);
    static void deserializeGridChargerTruckiConfig(JsonObject const& source, GridChargerTruckiConfig& target);

private:
    void loop();
    static double roundedFloat(float val);

    Task _loopTask;
};

extern ConfigurationClass Configuration;

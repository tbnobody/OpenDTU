// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "MessageOutput.h"
#include "defaults.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    JsonObject cfg = doc.createNestedObject("cfg");
    cfg["version"] = config.Cfg_Version;
    cfg["save_count"] = config.Cfg_SaveCount;

    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.WiFi_Ssid;
    wifi["password"] = config.WiFi_Password;
    wifi["ip"] = IPAddress(config.WiFi_Ip).toString();
    wifi["netmask"] = IPAddress(config.WiFi_Netmask).toString();
    wifi["gateway"] = IPAddress(config.WiFi_Gateway).toString();
    wifi["dns1"] = IPAddress(config.WiFi_Dns1).toString();
    wifi["dns2"] = IPAddress(config.WiFi_Dns2).toString();
    wifi["dhcp"] = config.WiFi_Dhcp;
    wifi["hostname"] = config.WiFi_Hostname;
    wifi["aptimeout"] = config.WiFi_ApTimeout;

    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["server"] = config.Ntp_Server;
    ntp["timezone"] = config.Ntp_Timezone;
    ntp["timezone_descr"] = config.Ntp_TimezoneDescr;
    ntp["latitude"] = config.Ntp_Latitude;
    ntp["longitude"] = config.Ntp_Longitude;
    ntp["sunsettype"] = config.Ntp_SunsetType;

    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["enabled"] = config.Mqtt_Enabled;
    mqtt["verbose_logging"] = config.Mqtt_VerboseLogging;
    mqtt["hostname"] = config.Mqtt_Hostname;
    mqtt["port"] = config.Mqtt_Port;
    mqtt["username"] = config.Mqtt_Username;
    mqtt["password"] = config.Mqtt_Password;
    mqtt["topic"] = config.Mqtt_Topic;
    mqtt["retain"] = config.Mqtt_Retain;
    mqtt["publish_interval"] = config.Mqtt_PublishInterval;
    mqtt["clean_session"] = config.Mqtt_CleanSession;

    JsonObject mqtt_lwt = mqtt.createNestedObject("lwt");
    mqtt_lwt["topic"] = config.Mqtt_LwtTopic;
    mqtt_lwt["value_online"] = config.Mqtt_LwtValue_Online;
    mqtt_lwt["value_offline"] = config.Mqtt_LwtValue_Offline;

    JsonObject mqtt_tls = mqtt.createNestedObject("tls");
    mqtt_tls["enabled"] = config.Mqtt_Tls;
    mqtt_tls["root_ca_cert"] = config.Mqtt_RootCaCert;
    mqtt_tls["certlogin"] = config.Mqtt_TlsCertLogin;
    mqtt_tls["client_cert"] = config.Mqtt_ClientCert;
    mqtt_tls["client_key"] = config.Mqtt_ClientKey;

    JsonObject mqtt_hass = mqtt.createNestedObject("hass");
    mqtt_hass["enabled"] = config.Mqtt_Hass_Enabled;
    mqtt_hass["retain"] = config.Mqtt_Hass_Retain;
    mqtt_hass["topic"] = config.Mqtt_Hass_Topic;
    mqtt_hass["individual_panels"] = config.Mqtt_Hass_IndividualPanels;
    mqtt_hass["expire"] = config.Mqtt_Hass_Expire;

    JsonObject dtu = doc.createNestedObject("dtu");
    dtu["serial"] = config.Dtu_Serial;
    dtu["poll_interval"] = config.Dtu_PollInterval;
    dtu["verbose_logging"] = config.Dtu_VerboseLogging;
    dtu["nrf_pa_level"] = config.Dtu_NrfPaLevel;
    dtu["cmt_pa_level"] = config.Dtu_CmtPaLevel;
    dtu["cmt_frequency"] = config.Dtu_CmtFrequency;

    JsonObject security = doc.createNestedObject("security");
    security["password"] = config.Security_Password;
    security["allow_readonly"] = config.Security_AllowReadonly;

    JsonObject device = doc.createNestedObject("device");
    device["pinmapping"] = config.Dev_PinMapping;

    JsonObject display = device.createNestedObject("display");
    display["powersafe"] = config.Display_PowerSafe;
    display["screensaver"] = config.Display_ScreenSaver;
    display["rotation"] = config.Display_Rotation;
    display["contrast"] = config.Display_Contrast;
    display["language"] = config.Display_Language;

    JsonArray inverters = doc.createNestedArray("inverters");
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        JsonObject inv = inverters.createNestedObject();
        inv["serial"] = config.Inverter[i].Serial;
        inv["name"] = config.Inverter[i].Name;
        inv["order"] = config.Inverter[i].Order;
        inv["poll_enable"] = config.Inverter[i].Poll_Enable;
        inv["poll_enable_night"] = config.Inverter[i].Poll_Enable_Night;
        inv["command_enable"] = config.Inverter[i].Command_Enable;
        inv["command_enable_night"] = config.Inverter[i].Command_Enable_Night;
        inv["reachable_threshold"] = config.Inverter[i].ReachableThreshold;
        inv["zero_runtime"] = config.Inverter[i].ZeroRuntimeDataIfUnrechable;
        inv["zero_day"] = config.Inverter[i].ZeroYieldDayOnMidnight;

        JsonArray channel = inv.createNestedArray("channel");
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            JsonObject chanData = channel.createNestedObject();
            chanData["name"] = config.Inverter[i].channel[c].Name;
            chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
            chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
        }
    }

    JsonObject vedirect = doc.createNestedObject("vedirect");
    vedirect["enabled"] = config.Vedirect_Enabled;
    vedirect["verbose_logging"] = config.Vedirect_VerboseLogging;
    vedirect["updates_only"] = config.Vedirect_UpdatesOnly;

    JsonObject powermeter = doc.createNestedObject("powermeter");
    powermeter["enabled"] = config.PowerMeter_Enabled;
    powermeter["verbose_logging"] = config.PowerMeter_VerboseLogging;
    powermeter["interval"] = config.PowerMeter_Interval;
    powermeter["source"] = config.PowerMeter_Source;
    powermeter["mqtt_topic_powermeter_1"] = config.PowerMeter_MqttTopicPowerMeter1;
    powermeter["mqtt_topic_powermeter_2"] = config.PowerMeter_MqttTopicPowerMeter2;
    powermeter["mqtt_topic_powermeter_3"] = config.PowerMeter_MqttTopicPowerMeter3;
    powermeter["sdmbaudrate"] = config.PowerMeter_SdmBaudrate;
    powermeter["sdmaddress"] = config.PowerMeter_SdmAddress;
    powermeter["http_individual_requests"] = config.PowerMeter_HttpIndividualRequests;

    JsonArray powermeter_http_phases = powermeter.createNestedArray("http_phases");
    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject powermeter_phase = powermeter_http_phases.createNestedObject();

        powermeter_phase["enabled"] = config.Powermeter_Http_Phase[i].Enabled;
        powermeter_phase["url"] = config.Powermeter_Http_Phase[i].Url;
        powermeter_phase["auth_type"] = config.Powermeter_Http_Phase[i].AuthType;
        powermeter_phase["username"] = config.Powermeter_Http_Phase[i].Username;
        powermeter_phase["password"] = config.Powermeter_Http_Phase[i].Password;
        powermeter_phase["header_key"] = config.Powermeter_Http_Phase[i].HeaderKey;
        powermeter_phase["header_value"] = config.Powermeter_Http_Phase[i].HeaderValue;
        powermeter_phase["timeout"] = config.Powermeter_Http_Phase[i].Timeout;
        powermeter_phase["json_path"] = config.Powermeter_Http_Phase[i].JsonPath;
    }

    JsonObject powerlimiter = doc.createNestedObject("powerlimiter");
    powerlimiter["enabled"] = config.PowerLimiter_Enabled;
    powerlimiter["verbose_logging"] = config.PowerLimiter_VerboseLogging;
    powerlimiter["solar_passtrough_enabled"] = config.PowerLimiter_SolarPassThroughEnabled;
    powerlimiter["solar_passtrough_losses"] = config.PowerLimiter_SolarPassThroughLosses;
    powerlimiter["battery_drain_strategy"] = config.PowerLimiter_BatteryDrainStategy;
    powerlimiter["interval"] = config.PowerLimiter_Interval;
    powerlimiter["is_inverter_behind_powermeter"] = config.PowerLimiter_IsInverterBehindPowerMeter;
    powerlimiter["inverter_id"] = config.PowerLimiter_InverterId;
    powerlimiter["inverter_channel_id"] = config.PowerLimiter_InverterChannelId;
    powerlimiter["target_power_consumption"] = config.PowerLimiter_TargetPowerConsumption;
    powerlimiter["target_power_consumption_hysteresis"] = config.PowerLimiter_TargetPowerConsumptionHysteresis;
    powerlimiter["lower_power_limit"] = config.PowerLimiter_LowerPowerLimit;
    powerlimiter["upper_power_limit"] = config.PowerLimiter_UpperPowerLimit;
    powerlimiter["battery_soc_start_threshold"] = config.PowerLimiter_BatterySocStartThreshold;
    powerlimiter["battery_soc_stop_threshold"] = config.PowerLimiter_BatterySocStopThreshold;
    powerlimiter["voltage_start_threshold"] = config.PowerLimiter_VoltageStartThreshold;
    powerlimiter["voltage_stop_threshold"] = config.PowerLimiter_VoltageStopThreshold;
    powerlimiter["voltage_load_correction_factor"] = config.PowerLimiter_VoltageLoadCorrectionFactor;
    powerlimiter["inverter_restart_hour"] = config.PowerLimiter_RestartHour;
    powerlimiter["full_solar_passthrough_soc"] = config.PowerLimiter_FullSolarPassThroughSoc;
    powerlimiter["full_solar_passthrough_start_voltage"] = config.PowerLimiter_FullSolarPassThroughStartVoltage;
    powerlimiter["full_solar_passthrough_stop_voltage"] = config.PowerLimiter_FullSolarPassThroughStopVoltage;

    JsonObject battery = doc.createNestedObject("battery");
    battery["enabled"] = config.Battery_Enabled;
    battery["verbose_logging"] = config.Battery_VerboseLogging;
    battery["provider"] = config.Battery_Provider;
    battery["jkbms_interface"] = config.Battery_JkBmsInterface;
    battery["jkbms_polling_interval"] = config.Battery_JkBmsPollingInterval;

    JsonObject huawei = doc.createNestedObject("huawei");
    huawei["enabled"] = config.Huawei_Enabled;
    huawei["can_controller_frequency"] = config.Huawei_CAN_Controller_Frequency;
    huawei["auto_power_enabled"] = config.Huawei_Auto_Power_Enabled;
    huawei["voltage_limit"] = config.Huawei_Auto_Power_Voltage_Limit;
    huawei["enable_voltage_limit"] = config.Huawei_Auto_Power_Enable_Voltage_Limit;
    huawei["lower_power_limit"] = config.Huawei_Auto_Power_Lower_Power_Limit;
    huawei["upper_power_limit"] = config.Huawei_Auto_Power_Upper_Power_Limit;

    // Serialize JSON to file
    if (serializeJson(doc, f) == 0) {
        MessageOutput.println("Failed to write file");
        return false;
    }

    f.close();
    return true;
}

bool ConfigurationClass::read()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    JsonObject cfg = doc["cfg"];
    config.Cfg_Version = cfg["version"] | CONFIG_VERSION;
    config.Cfg_SaveCount = cfg["save_count"] | 0;

    JsonObject wifi = doc["wifi"];
    strlcpy(config.WiFi_Ssid, wifi["ssid"] | WIFI_SSID, sizeof(config.WiFi_Ssid));
    strlcpy(config.WiFi_Password, wifi["password"] | WIFI_PASSWORD, sizeof(config.WiFi_Password));
    strlcpy(config.WiFi_Hostname, wifi["hostname"] | APP_HOSTNAME, sizeof(config.WiFi_Hostname));

    IPAddress wifi_ip;
    wifi_ip.fromString(wifi["ip"] | "");
    config.WiFi_Ip[0] = wifi_ip[0];
    config.WiFi_Ip[1] = wifi_ip[1];
    config.WiFi_Ip[2] = wifi_ip[2];
    config.WiFi_Ip[3] = wifi_ip[3];

    IPAddress wifi_netmask;
    wifi_netmask.fromString(wifi["netmask"] | "");
    config.WiFi_Netmask[0] = wifi_netmask[0];
    config.WiFi_Netmask[1] = wifi_netmask[1];
    config.WiFi_Netmask[2] = wifi_netmask[2];
    config.WiFi_Netmask[3] = wifi_netmask[3];

    IPAddress wifi_gateway;
    wifi_gateway.fromString(wifi["gateway"] | "");
    config.WiFi_Gateway[0] = wifi_gateway[0];
    config.WiFi_Gateway[1] = wifi_gateway[1];
    config.WiFi_Gateway[2] = wifi_gateway[2];
    config.WiFi_Gateway[3] = wifi_gateway[3];

    IPAddress wifi_dns1;
    wifi_dns1.fromString(wifi["dns1"] | "");
    config.WiFi_Dns1[0] = wifi_dns1[0];
    config.WiFi_Dns1[1] = wifi_dns1[1];
    config.WiFi_Dns1[2] = wifi_dns1[2];
    config.WiFi_Dns1[3] = wifi_dns1[3];

    IPAddress wifi_dns2;
    wifi_dns2.fromString(wifi["dns2"] | "");
    config.WiFi_Dns2[0] = wifi_dns2[0];
    config.WiFi_Dns2[1] = wifi_dns2[1];
    config.WiFi_Dns2[2] = wifi_dns2[2];
    config.WiFi_Dns2[3] = wifi_dns2[3];

    config.WiFi_Dhcp = wifi["dhcp"] | WIFI_DHCP;
    config.WiFi_ApTimeout = wifi["aptimeout"] | ACCESS_POINT_TIMEOUT;

    JsonObject ntp = doc["ntp"];
    strlcpy(config.Ntp_Server, ntp["server"] | NTP_SERVER, sizeof(config.Ntp_Server));
    strlcpy(config.Ntp_Timezone, ntp["timezone"] | NTP_TIMEZONE, sizeof(config.Ntp_Timezone));
    strlcpy(config.Ntp_TimezoneDescr, ntp["timezone_descr"] | NTP_TIMEZONEDESCR, sizeof(config.Ntp_TimezoneDescr));
    config.Ntp_Latitude = ntp["latitude"] | NTP_LATITUDE;
    config.Ntp_Longitude = ntp["longitude"] | NTP_LONGITUDE;
    config.Ntp_SunsetType = ntp["sunsettype"] | NTP_SUNSETTYPE;

    JsonObject mqtt = doc["mqtt"];
    config.Mqtt_Enabled = mqtt["enabled"] | MQTT_ENABLED;
    config.Mqtt_VerboseLogging = mqtt["verbose_logging"] | VERBOSE_LOGGING;
    strlcpy(config.Mqtt_Hostname, mqtt["hostname"] | MQTT_HOST, sizeof(config.Mqtt_Hostname));
    config.Mqtt_Port = mqtt["port"] | MQTT_PORT;
    strlcpy(config.Mqtt_Username, mqtt["username"] | MQTT_USER, sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, mqtt["password"] | MQTT_PASSWORD, sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, mqtt["topic"] | MQTT_TOPIC, sizeof(config.Mqtt_Topic));
    config.Mqtt_Retain = mqtt["retain"] | MQTT_RETAIN;
    config.Mqtt_PublishInterval = mqtt["publish_interval"] | MQTT_PUBLISH_INTERVAL;
    config.Mqtt_CleanSession = mqtt["clean_session"] | MQTT_CLEAN_SESSION;

    JsonObject mqtt_lwt = mqtt["lwt"];
    strlcpy(config.Mqtt_LwtTopic, mqtt_lwt["topic"] | MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, mqtt_lwt["value_online"] | MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, mqtt_lwt["value_offline"] | MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));

    JsonObject mqtt_tls = mqtt["tls"];
    config.Mqtt_Tls = mqtt_tls["enabled"] | MQTT_TLS;
    strlcpy(config.Mqtt_RootCaCert, mqtt_tls["root_ca_cert"] | MQTT_ROOT_CA_CERT, sizeof(config.Mqtt_RootCaCert));
    config.Mqtt_TlsCertLogin = mqtt_tls["certlogin"] | MQTT_TLSCERTLOGIN;
    strlcpy(config.Mqtt_ClientCert, mqtt_tls["client_cert"] | MQTT_TLSCLIENTCERT, sizeof(config.Mqtt_ClientCert));
    strlcpy(config.Mqtt_ClientKey, mqtt_tls["client_key"] | MQTT_TLSCLIENTKEY, sizeof(config.Mqtt_ClientKey));

    JsonObject mqtt_hass = mqtt["hass"];
    config.Mqtt_Hass_Enabled = mqtt_hass["enabled"] | MQTT_HASS_ENABLED;
    config.Mqtt_Hass_Retain = mqtt_hass["retain"] | MQTT_HASS_RETAIN;
    config.Mqtt_Hass_Expire = mqtt_hass["expire"] | MQTT_HASS_EXPIRE;
    config.Mqtt_Hass_IndividualPanels = mqtt_hass["individual_panels"] | MQTT_HASS_INDIVIDUALPANELS;
    strlcpy(config.Mqtt_Hass_Topic, mqtt_hass["topic"] | MQTT_HASS_TOPIC, sizeof(config.Mqtt_Hass_Topic));

    JsonObject dtu = doc["dtu"];
    config.Dtu_Serial = dtu["serial"] | DTU_SERIAL;
    config.Dtu_PollInterval = dtu["poll_interval"] | DTU_POLL_INTERVAL;
    config.Dtu_VerboseLogging = dtu["verbose_logging"] | VERBOSE_LOGGING;
    config.Dtu_NrfPaLevel = dtu["nrf_pa_level"] | DTU_NRF_PA_LEVEL;
    config.Dtu_CmtPaLevel = dtu["cmt_pa_level"] | DTU_CMT_PA_LEVEL;
    config.Dtu_CmtFrequency = dtu["cmt_frequency"] | DTU_CMT_FREQUENCY;

    JsonObject security = doc["security"];
    strlcpy(config.Security_Password, security["password"] | ACCESS_POINT_PASSWORD, sizeof(config.Security_Password));
    config.Security_AllowReadonly = security["allow_readonly"] | SECURITY_ALLOW_READONLY;

    JsonObject device = doc["device"];
    strlcpy(config.Dev_PinMapping, device["pinmapping"] | DEV_PINMAPPING, sizeof(config.Dev_PinMapping));

    JsonObject display = device["display"];
    config.Display_PowerSafe = display["powersafe"] | DISPLAY_POWERSAFE;
    config.Display_ScreenSaver = display["screensaver"] | DISPLAY_SCREENSAVER;
    config.Display_Rotation = display["rotation"] | DISPLAY_ROTATION;
    config.Display_Contrast = display["contrast"] | DISPLAY_CONTRAST;
    config.Display_Language = display["language"] | DISPLAY_LANGUAGE;

    JsonArray inverters = doc["inverters"];
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        JsonObject inv = inverters[i].as<JsonObject>();
        config.Inverter[i].Serial = inv["serial"] | 0ULL;
        strlcpy(config.Inverter[i].Name, inv["name"] | "", sizeof(config.Inverter[i].Name));
        config.Inverter[i].Order = inv["order"] | 0;

        config.Inverter[i].Poll_Enable = inv["poll_enable"] | true;
        config.Inverter[i].Poll_Enable_Night = inv["poll_enable_night"] | true;
        config.Inverter[i].Command_Enable = inv["command_enable"] | true;
        config.Inverter[i].Command_Enable_Night = inv["command_enable_night"] | true;
        config.Inverter[i].ReachableThreshold = inv["reachable_threshold"] | REACHABLE_THRESHOLD;
        config.Inverter[i].ZeroRuntimeDataIfUnrechable = inv["zero_runtime"] | false;
        config.Inverter[i].ZeroYieldDayOnMidnight = inv["zero_day"] | false;

        JsonArray channel = inv["channel"];
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            config.Inverter[i].channel[c].MaxChannelPower = channel[c]["max_power"] | 0;
            config.Inverter[i].channel[c].YieldTotalOffset = channel[c]["yield_total_offset"] | 0.0f;
            strlcpy(config.Inverter[i].channel[c].Name, channel[c]["name"] | "", sizeof(config.Inverter[i].channel[c].Name));
        }
    }

    JsonObject vedirect = doc["vedirect"];
    config.Vedirect_Enabled = vedirect["enabled"] | VEDIRECT_ENABLED;
    config.Vedirect_VerboseLogging = vedirect["verbose_logging"] | VEDIRECT_VERBOSE_LOGGING;
    config.Vedirect_UpdatesOnly = vedirect["updates_only"] | VEDIRECT_UPDATESONLY;

    JsonObject powermeter = doc["powermeter"];
    config.PowerMeter_Enabled = powermeter["enabled"] | POWERMETER_ENABLED;
    config.PowerMeter_VerboseLogging = powermeter["verbose_logging"] | VERBOSE_LOGGING;
    config.PowerMeter_Interval =  powermeter["interval"] | POWERMETER_INTERVAL;
    config.PowerMeter_Source =  powermeter["source"] | POWERMETER_SOURCE;
    strlcpy(config.PowerMeter_MqttTopicPowerMeter1, powermeter["mqtt_topic_powermeter_1"] | "", sizeof(config.PowerMeter_MqttTopicPowerMeter1));
    strlcpy(config.PowerMeter_MqttTopicPowerMeter2, powermeter["mqtt_topic_powermeter_2"] | "", sizeof(config.PowerMeter_MqttTopicPowerMeter2));
    strlcpy(config.PowerMeter_MqttTopicPowerMeter3, powermeter["mqtt_topic_powermeter_3"] | "", sizeof(config.PowerMeter_MqttTopicPowerMeter3));
    config.PowerMeter_SdmBaudrate =  powermeter["sdmbaudrate"] | POWERMETER_SDMBAUDRATE;
    config.PowerMeter_SdmAddress =  powermeter["sdmaddress"] | POWERMETER_SDMADDRESS;
    config.PowerMeter_HttpIndividualRequests = powermeter["http_individual_requests"] | false;

    JsonArray powermeter_http_phases = powermeter["http_phases"];
    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject powermeter_phase = powermeter_http_phases[i].as<JsonObject>();

        config.Powermeter_Http_Phase[i].Enabled = powermeter_phase["enabled"] | (i == 0);
        strlcpy(config.Powermeter_Http_Phase[i].Url, powermeter_phase["url"] | "", sizeof(config.Powermeter_Http_Phase[i].Url));
        config.Powermeter_Http_Phase[i].AuthType = powermeter_phase["auth_type"] | Auth::none;
        strlcpy(config.Powermeter_Http_Phase[i].Username, powermeter_phase["username"] | "", sizeof(config.Powermeter_Http_Phase[i].Username));
        strlcpy(config.Powermeter_Http_Phase[i].Password, powermeter_phase["password"] | "", sizeof(config.Powermeter_Http_Phase[i].Password));
        strlcpy(config.Powermeter_Http_Phase[i].HeaderKey, powermeter_phase["header_key"] | "", sizeof(config.Powermeter_Http_Phase[i].HeaderKey));
        strlcpy(config.Powermeter_Http_Phase[i].HeaderValue, powermeter_phase["header_value"] | "", sizeof(config.Powermeter_Http_Phase[i].HeaderValue));
        config.Powermeter_Http_Phase[i].Timeout = powermeter_phase["timeout"] | POWERMETER_HTTP_TIMEOUT;
        strlcpy(config.Powermeter_Http_Phase[i].JsonPath, powermeter_phase["json_path"] | "", sizeof(config.Powermeter_Http_Phase[i].JsonPath));
    }

    JsonObject powerlimiter = doc["powerlimiter"];
    config.PowerLimiter_Enabled = powerlimiter["enabled"] | POWERLIMITER_ENABLED;
    config.PowerLimiter_VerboseLogging = powerlimiter["verbose_logging"] | VERBOSE_LOGGING;
    config.PowerLimiter_SolarPassThroughEnabled = powerlimiter["solar_passtrough_enabled"] | POWERLIMITER_SOLAR_PASSTHROUGH_ENABLED;
    config.PowerLimiter_SolarPassThroughLosses = powerlimiter["solar_passthrough_losses"] | POWERLIMITER_SOLAR_PASSTHROUGH_LOSSES;
    config.PowerLimiter_BatteryDrainStategy = powerlimiter["battery_drain_strategy"] | POWERLIMITER_BATTERY_DRAIN_STRATEGY;
    config.PowerLimiter_Interval =  powerlimiter["interval"] | POWERLIMITER_INTERVAL;
    config.PowerLimiter_IsInverterBehindPowerMeter = powerlimiter["is_inverter_behind_powermeter"] | POWERLIMITER_IS_INVERTER_BEHIND_POWER_METER;
    config.PowerLimiter_InverterId = powerlimiter["inverter_id"] | POWERLIMITER_INVERTER_ID;
    config.PowerLimiter_InverterChannelId = powerlimiter["inverter_channel_id"] | POWERLIMITER_INVERTER_CHANNEL_ID;
    config.PowerLimiter_TargetPowerConsumption = powerlimiter["target_power_consumption"] | POWERLIMITER_TARGET_POWER_CONSUMPTION;
    config.PowerLimiter_TargetPowerConsumptionHysteresis = powerlimiter["target_power_consumption_hysteresis"] | POWERLIMITER_TARGET_POWER_CONSUMPTION_HYSTERESIS;
    config.PowerLimiter_LowerPowerLimit = powerlimiter["lower_power_limit"] | POWERLIMITER_LOWER_POWER_LIMIT;
    config.PowerLimiter_UpperPowerLimit = powerlimiter["upper_power_limit"] | POWERLIMITER_UPPER_POWER_LIMIT;
    config.PowerLimiter_BatterySocStartThreshold = powerlimiter["battery_soc_start_threshold"] | POWERLIMITER_BATTERY_SOC_START_THRESHOLD;
    config.PowerLimiter_BatterySocStopThreshold = powerlimiter["battery_soc_stop_threshold"] | POWERLIMITER_BATTERY_SOC_STOP_THRESHOLD;
    config.PowerLimiter_VoltageStartThreshold = powerlimiter["voltage_start_threshold"] | POWERLIMITER_VOLTAGE_START_THRESHOLD;
    config.PowerLimiter_VoltageStopThreshold = powerlimiter["voltage_stop_threshold"] | POWERLIMITER_VOLTAGE_STOP_THRESHOLD;
    config.PowerLimiter_VoltageLoadCorrectionFactor = powerlimiter["voltage_load_correction_factor"] | POWERLIMITER_VOLTAGE_LOAD_CORRECTION_FACTOR;
    config.PowerLimiter_RestartHour = powerlimiter["inverter_restart_hour"] | POWERLIMITER_RESTART_HOUR;
    config.PowerLimiter_FullSolarPassThroughSoc = powerlimiter["full_solar_passthrough_soc"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_SOC;
    config.PowerLimiter_FullSolarPassThroughStartVoltage = powerlimiter["full_solar_passthrough_start_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_START_VOLTAGE;
    config.PowerLimiter_FullSolarPassThroughStopVoltage = powerlimiter["full_solar_passthrough_stop_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_STOP_VOLTAGE;

    JsonObject battery = doc["battery"];
    config.Battery_Enabled = battery["enabled"] | BATTERY_ENABLED;
    config.Battery_VerboseLogging = battery["verbose_logging"] | VERBOSE_LOGGING;
    config.Battery_Provider = battery["provider"] | BATTERY_PROVIDER;
    config.Battery_JkBmsInterface = battery["jkbms_interface"] | BATTERY_JKBMS_INTERFACE;
    config.Battery_JkBmsPollingInterval = battery["jkbms_polling_interval"] | BATTERY_JKBMS_POLLING_INTERVAL;

    JsonObject huawei = doc["huawei"];
    config.Huawei_Enabled = huawei["enabled"] | HUAWEI_ENABLED;
    config.Huawei_CAN_Controller_Frequency = huawei["can_controller_frequency"] | HUAWEI_CAN_CONTROLLER_FREQUENCY;
    config.Huawei_Auto_Power_Enabled = huawei["auto_power_enabled"] | false;
    config.Huawei_Auto_Power_Voltage_Limit = huawei["voltage_limit"] | HUAWEI_AUTO_POWER_VOLTAGE_LIMIT;
    config.Huawei_Auto_Power_Enable_Voltage_Limit =  huawei["enable_voltage_limit"] | HUAWEI_AUTO_POWER_ENABLE_VOLTAGE_LIMIT;
    config.Huawei_Auto_Power_Lower_Power_Limit = huawei["lower_power_limit"] | HUAWEI_AUTO_POWER_LOWER_POWER_LIMIT;
    config.Huawei_Auto_Power_Upper_Power_Limit = huawei["upper_power_limit"] | HUAWEI_AUTO_POWER_UPPER_POWER_LIMIT;

    f.close();
    return true;
}

void ConfigurationClass::migrate()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);
    if (!f) {
        MessageOutput.println("Failed to open file, cancel migration");
        return;
    }

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.printf("Failed to read file, cancel migration: %s\r\n", error.c_str());
        return;
    }

    if (config.Cfg_Version < 0x00011700) {
        JsonArray inverters = doc["inverters"];
        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            JsonObject inv = inverters[i].as<JsonObject>();
            JsonArray channels = inv["channels"];
            for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                config.Inverter[i].channel[c].MaxChannelPower = channels[c];
                strlcpy(config.Inverter[i].channel[c].Name, "", sizeof(config.Inverter[i].channel[c].Name));
            }
        }
    }

    if (config.Cfg_Version < 0x00011800) {
        JsonObject mqtt = doc["mqtt"];
        config.Mqtt_PublishInterval = mqtt["publish_invterval"];
    }

    if (config.Cfg_Version < 0x00011900) {
        JsonObject dtu = doc["dtu"];
        config.Dtu_NrfPaLevel = dtu["pa_level"];
    }

    f.close();

    config.Cfg_Version = CONFIG_VERSION;
    write();
    read();
}

CONFIG_T& ConfigurationClass::get()
{
    return config;
}

INVERTER_CONFIG_T* ConfigurationClass::getFreeInverterSlot()
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == 0) {
            return &config.Inverter[i];
        }
    }

    return NULL;
}

INVERTER_CONFIG_T* ConfigurationClass::getInverterConfig(uint64_t serial)
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == serial) {
            return &config.Inverter[i];
        }
    }

    return NULL;
}

ConfigurationClass Configuration;

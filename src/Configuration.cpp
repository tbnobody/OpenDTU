// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "defaults.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <nvs_flash.h>

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
    config.Cfg.SaveCount++;

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
    }

    JsonObject cfg = doc.createNestedObject("cfg");
    cfg["version"] = config.Cfg.Version;
    cfg["save_count"] = config.Cfg.SaveCount;

    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = config.WiFi.Ssid;
    wifi["password"] = config.WiFi.Password;
    wifi["ip"] = IPAddress(config.WiFi.Ip).toString();
    wifi["netmask"] = IPAddress(config.WiFi.Netmask).toString();
    wifi["gateway"] = IPAddress(config.WiFi.Gateway).toString();
    wifi["dns1"] = IPAddress(config.WiFi.Dns1).toString();
    wifi["dns2"] = IPAddress(config.WiFi.Dns2).toString();
    wifi["dhcp"] = config.WiFi.Dhcp;
    wifi["hostname"] = config.WiFi.Hostname;
    wifi["aptimeout"] = config.WiFi.ApTimeout;

    JsonObject mdns = doc.createNestedObject("mdns");
    mdns["enabled"] = config.Mdns.Enabled;

    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["server"] = config.Ntp.Server;
    ntp["timezone"] = config.Ntp.Timezone;
    ntp["timezone_descr"] = config.Ntp.TimezoneDescr;
    ntp["latitude"] = config.Ntp.Latitude;
    ntp["longitude"] = config.Ntp.Longitude;
    ntp["sunsettype"] = config.Ntp.SunsetType;

    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["enabled"] = config.Mqtt.Enabled;
    mqtt["verbose_logging"] = config.Mqtt.VerboseLogging;
    mqtt["hostname"] = config.Mqtt.Hostname;
    mqtt["port"] = config.Mqtt.Port;
    mqtt["username"] = config.Mqtt.Username;
    mqtt["password"] = config.Mqtt.Password;
    mqtt["topic"] = config.Mqtt.Topic;
    mqtt["retain"] = config.Mqtt.Retain;
    mqtt["publish_interval"] = config.Mqtt.PublishInterval;
    mqtt["clean_session"] = config.Mqtt.CleanSession;

    JsonObject mqtt_lwt = mqtt.createNestedObject("lwt");
    mqtt_lwt["topic"] = config.Mqtt.Lwt.Topic;
    mqtt_lwt["value_online"] = config.Mqtt.Lwt.Value_Online;
    mqtt_lwt["value_offline"] = config.Mqtt.Lwt.Value_Offline;
    mqtt_lwt["qos"] = config.Mqtt.Lwt.Qos;

    JsonObject mqtt_tls = mqtt.createNestedObject("tls");
    mqtt_tls["enabled"] = config.Mqtt.Tls.Enabled;
    mqtt_tls["root_ca_cert"] = config.Mqtt.Tls.RootCaCert;
    mqtt_tls["certlogin"] = config.Mqtt.Tls.CertLogin;
    mqtt_tls["client_cert"] = config.Mqtt.Tls.ClientCert;
    mqtt_tls["client_key"] = config.Mqtt.Tls.ClientKey;

    JsonObject mqtt_hass = mqtt.createNestedObject("hass");
    mqtt_hass["enabled"] = config.Mqtt.Hass.Enabled;
    mqtt_hass["retain"] = config.Mqtt.Hass.Retain;
    mqtt_hass["topic"] = config.Mqtt.Hass.Topic;
    mqtt_hass["individual_panels"] = config.Mqtt.Hass.IndividualPanels;
    mqtt_hass["expire"] = config.Mqtt.Hass.Expire;

    JsonObject dtu = doc.createNestedObject("dtu");
    dtu["serial"] = config.Dtu.Serial;
    dtu["poll_interval"] = config.Dtu.PollInterval;
    dtu["verbose_logging"] = config.Dtu.VerboseLogging;
    dtu["nrf_pa_level"] = config.Dtu.Nrf.PaLevel;
    dtu["cmt_pa_level"] = config.Dtu.Cmt.PaLevel;
    dtu["cmt_frequency"] = config.Dtu.Cmt.Frequency;

    JsonObject security = doc.createNestedObject("security");
    security["password"] = config.Security.Password;
    security["allow_readonly"] = config.Security.AllowReadonly;

    JsonObject device = doc.createNestedObject("device");
    device["pinmapping"] = config.Dev_PinMapping;

    JsonObject display = device.createNestedObject("display");
    display["powersafe"] = config.Display.PowerSafe;
    display["screensaver"] = config.Display.ScreenSaver;
    display["rotation"] = config.Display.Rotation;
    display["contrast"] = config.Display.Contrast;
    display["language"] = config.Display.Language;
    display["diagram_duration"] = config.Display.DiagramDuration;

    JsonArray leds = device.createNestedArray("led");
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds.createNestedObject();
        led["brightness"] = config.Led_Single[i].Brightness;
    }

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
        inv["yieldday_correction"] = config.Inverter[i].YieldDayCorrection;

        JsonArray channel = inv.createNestedArray("channel");
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            JsonObject chanData = channel.createNestedObject();
            chanData["name"] = config.Inverter[i].channel[c].Name;
            chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
            chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
        }
    }

    JsonObject vedirect = doc.createNestedObject("vedirect");
    vedirect["enabled"] = config.Vedirect.Enabled;
    vedirect["verbose_logging"] = config.Vedirect.VerboseLogging;
    vedirect["updates_only"] = config.Vedirect.UpdatesOnly;

    JsonObject powermeter = doc.createNestedObject("powermeter");
    powermeter["enabled"] = config.PowerMeter.Enabled;
    powermeter["verbose_logging"] = config.PowerMeter.VerboseLogging;
    powermeter["interval"] = config.PowerMeter.Interval;
    powermeter["source"] = config.PowerMeter.Source;
    powermeter["mqtt_topic_powermeter_1"] = config.PowerMeter.MqttTopicPowerMeter1;
    powermeter["mqtt_topic_powermeter_2"] = config.PowerMeter.MqttTopicPowerMeter2;
    powermeter["mqtt_topic_powermeter_3"] = config.PowerMeter.MqttTopicPowerMeter3;
    powermeter["sdmbaudrate"] = config.PowerMeter.SdmBaudrate;
    powermeter["sdmaddress"] = config.PowerMeter.SdmAddress;
    powermeter["http_individual_requests"] = config.PowerMeter.HttpIndividualRequests;

    JsonArray powermeter_http_phases = powermeter.createNestedArray("http_phases");
    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject powermeter_phase = powermeter_http_phases.createNestedObject();

        powermeter_phase["enabled"] = config.PowerMeter.Http_Phase[i].Enabled;
        powermeter_phase["url"] = config.PowerMeter.Http_Phase[i].Url;
        powermeter_phase["auth_type"] = config.PowerMeter.Http_Phase[i].AuthType;
        powermeter_phase["username"] = config.PowerMeter.Http_Phase[i].Username;
        powermeter_phase["password"] = config.PowerMeter.Http_Phase[i].Password;
        powermeter_phase["header_key"] = config.PowerMeter.Http_Phase[i].HeaderKey;
        powermeter_phase["header_value"] = config.PowerMeter.Http_Phase[i].HeaderValue;
        powermeter_phase["timeout"] = config.PowerMeter.Http_Phase[i].Timeout;
        powermeter_phase["json_path"] = config.PowerMeter.Http_Phase[i].JsonPath;
    }

    JsonObject powerlimiter = doc.createNestedObject("powerlimiter");
    powerlimiter["enabled"] = config.PowerLimiter.Enabled;
    powerlimiter["verbose_logging"] = config.PowerLimiter.VerboseLogging;
    powerlimiter["solar_passtrough_enabled"] = config.PowerLimiter.SolarPassThroughEnabled;
    powerlimiter["solar_passtrough_losses"] = config.PowerLimiter.SolarPassThroughLosses;
    powerlimiter["battery_drain_strategy"] = config.PowerLimiter.BatteryDrainStategy;
    powerlimiter["interval"] = config.PowerLimiter.Interval;
    powerlimiter["is_inverter_behind_powermeter"] = config.PowerLimiter.IsInverterBehindPowerMeter;
    powerlimiter["inverter_id"] = config.PowerLimiter.InverterId;
    powerlimiter["inverter_channel_id"] = config.PowerLimiter.InverterChannelId;
    powerlimiter["target_power_consumption"] = config.PowerLimiter.TargetPowerConsumption;
    powerlimiter["target_power_consumption_hysteresis"] = config.PowerLimiter.TargetPowerConsumptionHysteresis;
    powerlimiter["lower_power_limit"] = config.PowerLimiter.LowerPowerLimit;
    powerlimiter["upper_power_limit"] = config.PowerLimiter.UpperPowerLimit;
    powerlimiter["battery_soc_start_threshold"] = config.PowerLimiter.BatterySocStartThreshold;
    powerlimiter["battery_soc_stop_threshold"] = config.PowerLimiter.BatterySocStopThreshold;
    powerlimiter["voltage_start_threshold"] = config.PowerLimiter.VoltageStartThreshold;
    powerlimiter["voltage_stop_threshold"] = config.PowerLimiter.VoltageStopThreshold;
    powerlimiter["voltage_load_correction_factor"] = config.PowerLimiter.VoltageLoadCorrectionFactor;
    powerlimiter["inverter_restart_hour"] = config.PowerLimiter.RestartHour;
    powerlimiter["full_solar_passthrough_soc"] = config.PowerLimiter.FullSolarPassThroughSoc;
    powerlimiter["full_solar_passthrough_start_voltage"] = config.PowerLimiter.FullSolarPassThroughStartVoltage;
    powerlimiter["full_solar_passthrough_stop_voltage"] = config.PowerLimiter.FullSolarPassThroughStopVoltage;

    JsonObject battery = doc.createNestedObject("battery");
    battery["enabled"] = config.Battery.Enabled;
    battery["verbose_logging"] = config.Battery.VerboseLogging;
    battery["provider"] = config.Battery.Provider;
    battery["jkbms_interface"] = config.Battery.JkBmsInterface;
    battery["jkbms_polling_interval"] = config.Battery.JkBmsPollingInterval;
    battery["mqtt_topic"] = config.Battery.MqttTopic;

    JsonObject huawei = doc.createNestedObject("huawei");
    huawei["enabled"] = config.Huawei.Enabled;
    huawei["can_controller_frequency"] = config.Huawei.CAN_Controller_Frequency;
    huawei["auto_power_enabled"] = config.Huawei.Auto_Power_Enabled;
    huawei["voltage_limit"] = config.Huawei.Auto_Power_Voltage_Limit;
    huawei["enable_voltage_limit"] = config.Huawei.Auto_Power_Enable_Voltage_Limit;
    huawei["lower_power_limit"] = config.Huawei.Auto_Power_Lower_Power_Limit;
    huawei["upper_power_limit"] = config.Huawei.Auto_Power_Upper_Power_Limit;

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

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
    }

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    JsonObject cfg = doc["cfg"];
    config.Cfg.Version = cfg["version"] | CONFIG_VERSION;
    config.Cfg.SaveCount = cfg["save_count"] | 0;

    JsonObject wifi = doc["wifi"];
    strlcpy(config.WiFi.Ssid, wifi["ssid"] | WIFI_SSID, sizeof(config.WiFi.Ssid));
    strlcpy(config.WiFi.Password, wifi["password"] | WIFI_PASSWORD, sizeof(config.WiFi.Password));
    strlcpy(config.WiFi.Hostname, wifi["hostname"] | APP_HOSTNAME, sizeof(config.WiFi.Hostname));

    IPAddress wifi_ip;
    wifi_ip.fromString(wifi["ip"] | "");
    config.WiFi.Ip[0] = wifi_ip[0];
    config.WiFi.Ip[1] = wifi_ip[1];
    config.WiFi.Ip[2] = wifi_ip[2];
    config.WiFi.Ip[3] = wifi_ip[3];

    IPAddress wifi_netmask;
    wifi_netmask.fromString(wifi["netmask"] | "");
    config.WiFi.Netmask[0] = wifi_netmask[0];
    config.WiFi.Netmask[1] = wifi_netmask[1];
    config.WiFi.Netmask[2] = wifi_netmask[2];
    config.WiFi.Netmask[3] = wifi_netmask[3];

    IPAddress wifi_gateway;
    wifi_gateway.fromString(wifi["gateway"] | "");
    config.WiFi.Gateway[0] = wifi_gateway[0];
    config.WiFi.Gateway[1] = wifi_gateway[1];
    config.WiFi.Gateway[2] = wifi_gateway[2];
    config.WiFi.Gateway[3] = wifi_gateway[3];

    IPAddress wifi_dns1;
    wifi_dns1.fromString(wifi["dns1"] | "");
    config.WiFi.Dns1[0] = wifi_dns1[0];
    config.WiFi.Dns1[1] = wifi_dns1[1];
    config.WiFi.Dns1[2] = wifi_dns1[2];
    config.WiFi.Dns1[3] = wifi_dns1[3];

    IPAddress wifi_dns2;
    wifi_dns2.fromString(wifi["dns2"] | "");
    config.WiFi.Dns2[0] = wifi_dns2[0];
    config.WiFi.Dns2[1] = wifi_dns2[1];
    config.WiFi.Dns2[2] = wifi_dns2[2];
    config.WiFi.Dns2[3] = wifi_dns2[3];

    config.WiFi.Dhcp = wifi["dhcp"] | WIFI_DHCP;
    config.WiFi.ApTimeout = wifi["aptimeout"] | ACCESS_POINT_TIMEOUT;

    JsonObject mdns = doc["mdns"];
    config.Mdns.Enabled = mdns["enabled"] | MDNS_ENABLED;

    JsonObject ntp = doc["ntp"];
    strlcpy(config.Ntp.Server, ntp["server"] | NTP_SERVER, sizeof(config.Ntp.Server));
    strlcpy(config.Ntp.Timezone, ntp["timezone"] | NTP_TIMEZONE, sizeof(config.Ntp.Timezone));
    strlcpy(config.Ntp.TimezoneDescr, ntp["timezone_descr"] | NTP_TIMEZONEDESCR, sizeof(config.Ntp.TimezoneDescr));
    config.Ntp.Latitude = ntp["latitude"] | NTP_LATITUDE;
    config.Ntp.Longitude = ntp["longitude"] | NTP_LONGITUDE;
    config.Ntp.SunsetType = ntp["sunsettype"] | NTP_SUNSETTYPE;

    JsonObject mqtt = doc["mqtt"];
    config.Mqtt.Enabled = mqtt["enabled"] | MQTT_ENABLED;
    config.Mqtt.VerboseLogging = mqtt["verbose_logging"] | VERBOSE_LOGGING;
    strlcpy(config.Mqtt.Hostname, mqtt["hostname"] | MQTT_HOST, sizeof(config.Mqtt.Hostname));
    config.Mqtt.Port = mqtt["port"] | MQTT_PORT;
    strlcpy(config.Mqtt.Username, mqtt["username"] | MQTT_USER, sizeof(config.Mqtt.Username));
    strlcpy(config.Mqtt.Password, mqtt["password"] | MQTT_PASSWORD, sizeof(config.Mqtt.Password));
    strlcpy(config.Mqtt.Topic, mqtt["topic"] | MQTT_TOPIC, sizeof(config.Mqtt.Topic));
    config.Mqtt.Retain = mqtt["retain"] | MQTT_RETAIN;
    config.Mqtt.PublishInterval = mqtt["publish_interval"] | MQTT_PUBLISH_INTERVAL;
    config.Mqtt.CleanSession = mqtt["clean_session"] | MQTT_CLEAN_SESSION;

    JsonObject mqtt_lwt = mqtt["lwt"];
    strlcpy(config.Mqtt.Lwt.Topic, mqtt_lwt["topic"] | MQTT_LWT_TOPIC, sizeof(config.Mqtt.Lwt.Topic));
    strlcpy(config.Mqtt.Lwt.Value_Online, mqtt_lwt["value_online"] | MQTT_LWT_ONLINE, sizeof(config.Mqtt.Lwt.Value_Online));
    strlcpy(config.Mqtt.Lwt.Value_Offline, mqtt_lwt["value_offline"] | MQTT_LWT_OFFLINE, sizeof(config.Mqtt.Lwt.Value_Offline));
    config.Mqtt.Lwt.Qos = mqtt_lwt["qos"] | MQTT_LWT_QOS;

    JsonObject mqtt_tls = mqtt["tls"];
    config.Mqtt.Tls.Enabled = mqtt_tls["enabled"] | MQTT_TLS;
    strlcpy(config.Mqtt.Tls.RootCaCert, mqtt_tls["root_ca_cert"] | MQTT_ROOT_CA_CERT, sizeof(config.Mqtt.Tls.RootCaCert));
    config.Mqtt.Tls.CertLogin = mqtt_tls["certlogin"] | MQTT_TLSCERTLOGIN;
    strlcpy(config.Mqtt.Tls.ClientCert, mqtt_tls["client_cert"] | MQTT_TLSCLIENTCERT, sizeof(config.Mqtt.Tls.ClientCert));
    strlcpy(config.Mqtt.Tls.ClientKey, mqtt_tls["client_key"] | MQTT_TLSCLIENTKEY, sizeof(config.Mqtt.Tls.ClientKey));

    JsonObject mqtt_hass = mqtt["hass"];
    config.Mqtt.Hass.Enabled = mqtt_hass["enabled"] | MQTT_HASS_ENABLED;
    config.Mqtt.Hass.Retain = mqtt_hass["retain"] | MQTT_HASS_RETAIN;
    config.Mqtt.Hass.Expire = mqtt_hass["expire"] | MQTT_HASS_EXPIRE;
    config.Mqtt.Hass.IndividualPanels = mqtt_hass["individual_panels"] | MQTT_HASS_INDIVIDUALPANELS;
    strlcpy(config.Mqtt.Hass.Topic, mqtt_hass["topic"] | MQTT_HASS_TOPIC, sizeof(config.Mqtt.Hass.Topic));

    JsonObject dtu = doc["dtu"];
    config.Dtu.Serial = dtu["serial"] | DTU_SERIAL;
    config.Dtu.PollInterval = dtu["poll_interval"] | DTU_POLL_INTERVAL;
    config.Dtu.VerboseLogging = dtu["verbose_logging"] | VERBOSE_LOGGING;
    config.Dtu.Nrf.PaLevel = dtu["nrf_pa_level"] | DTU_NRF_PA_LEVEL;
    config.Dtu.Cmt.PaLevel = dtu["cmt_pa_level"] | DTU_CMT_PA_LEVEL;
    config.Dtu.Cmt.Frequency = dtu["cmt_frequency"] | DTU_CMT_FREQUENCY;

    JsonObject security = doc["security"];
    strlcpy(config.Security.Password, security["password"] | ACCESS_POINT_PASSWORD, sizeof(config.Security.Password));
    config.Security.AllowReadonly = security["allow_readonly"] | SECURITY_ALLOW_READONLY;

    JsonObject device = doc["device"];
    strlcpy(config.Dev_PinMapping, device["pinmapping"] | DEV_PINMAPPING, sizeof(config.Dev_PinMapping));

    JsonObject display = device["display"];
    config.Display.PowerSafe = display["powersafe"] | DISPLAY_POWERSAFE;
    config.Display.ScreenSaver = display["screensaver"] | DISPLAY_SCREENSAVER;
    config.Display.Rotation = display["rotation"] | DISPLAY_ROTATION;
    config.Display.Contrast = display["contrast"] | DISPLAY_CONTRAST;
    config.Display.Language = display["language"] | DISPLAY_LANGUAGE;
    config.Display.DiagramDuration = display["diagram_duration"] | DISPLAY_DIAGRAM_DURATION;

    JsonArray leds = device["led"];
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds[i].as<JsonObject>();
        config.Led_Single[i].Brightness = led["brightness"] | LED_BRIGHTNESS;
    }

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
        config.Inverter[i].YieldDayCorrection = inv["yieldday_correction"] | false;

        JsonArray channel = inv["channel"];
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            config.Inverter[i].channel[c].MaxChannelPower = channel[c]["max_power"] | 0;
            config.Inverter[i].channel[c].YieldTotalOffset = channel[c]["yield_total_offset"] | 0.0f;
            strlcpy(config.Inverter[i].channel[c].Name, channel[c]["name"] | "", sizeof(config.Inverter[i].channel[c].Name));
        }
    }

    JsonObject vedirect = doc["vedirect"];
    config.Vedirect.Enabled = vedirect["enabled"] | VEDIRECT_ENABLED;
    config.Vedirect.VerboseLogging = vedirect["verbose_logging"] | VEDIRECT_VERBOSE_LOGGING;
    config.Vedirect.UpdatesOnly = vedirect["updates_only"] | VEDIRECT_UPDATESONLY;

    JsonObject powermeter = doc["powermeter"];
    config.PowerMeter.Enabled = powermeter["enabled"] | POWERMETER_ENABLED;
    config.PowerMeter.VerboseLogging = powermeter["verbose_logging"] | VERBOSE_LOGGING;
    config.PowerMeter.Interval =  powermeter["interval"] | POWERMETER_INTERVAL;
    config.PowerMeter.Source =  powermeter["source"] | POWERMETER_SOURCE;
    strlcpy(config.PowerMeter.MqttTopicPowerMeter1, powermeter["mqtt_topic_powermeter_1"] | "", sizeof(config.PowerMeter.MqttTopicPowerMeter1));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter2, powermeter["mqtt_topic_powermeter_2"] | "", sizeof(config.PowerMeter.MqttTopicPowerMeter2));
    strlcpy(config.PowerMeter.MqttTopicPowerMeter3, powermeter["mqtt_topic_powermeter_3"] | "", sizeof(config.PowerMeter.MqttTopicPowerMeter3));
    config.PowerMeter.SdmBaudrate =  powermeter["sdmbaudrate"] | POWERMETER_SDMBAUDRATE;
    config.PowerMeter.SdmAddress =  powermeter["sdmaddress"] | POWERMETER_SDMADDRESS;
    config.PowerMeter.HttpIndividualRequests = powermeter["http_individual_requests"] | false;

    JsonArray powermeter_http_phases = powermeter["http_phases"];
    for (uint8_t i = 0; i < POWERMETER_MAX_PHASES; i++) {
        JsonObject powermeter_phase = powermeter_http_phases[i].as<JsonObject>();

        config.PowerMeter.Http_Phase[i].Enabled = powermeter_phase["enabled"] | (i == 0);
        strlcpy(config.PowerMeter.Http_Phase[i].Url, powermeter_phase["url"] | "", sizeof(config.PowerMeter.Http_Phase[i].Url));
        config.PowerMeter.Http_Phase[i].AuthType = powermeter_phase["auth_type"] | Auth::none;
        strlcpy(config.PowerMeter.Http_Phase[i].Username, powermeter_phase["username"] | "", sizeof(config.PowerMeter.Http_Phase[i].Username));
        strlcpy(config.PowerMeter.Http_Phase[i].Password, powermeter_phase["password"] | "", sizeof(config.PowerMeter.Http_Phase[i].Password));
        strlcpy(config.PowerMeter.Http_Phase[i].HeaderKey, powermeter_phase["header_key"] | "", sizeof(config.PowerMeter.Http_Phase[i].HeaderKey));
        strlcpy(config.PowerMeter.Http_Phase[i].HeaderValue, powermeter_phase["header_value"] | "", sizeof(config.PowerMeter.Http_Phase[i].HeaderValue));
        config.PowerMeter.Http_Phase[i].Timeout = powermeter_phase["timeout"] | POWERMETER_HTTP_TIMEOUT;
        strlcpy(config.PowerMeter.Http_Phase[i].JsonPath, powermeter_phase["json_path"] | "", sizeof(config.PowerMeter.Http_Phase[i].JsonPath));
    }

    JsonObject powerlimiter = doc["powerlimiter"];
    config.PowerLimiter.Enabled = powerlimiter["enabled"] | POWERLIMITER_ENABLED;
    config.PowerLimiter.VerboseLogging = powerlimiter["verbose_logging"] | VERBOSE_LOGGING;
    config.PowerLimiter.SolarPassThroughEnabled = powerlimiter["solar_passtrough_enabled"] | POWERLIMITER_SOLAR_PASSTHROUGH_ENABLED;
    config.PowerLimiter.SolarPassThroughLosses = powerlimiter["solar_passthrough_losses"] | POWERLIMITER_SOLAR_PASSTHROUGH_LOSSES;
    config.PowerLimiter.BatteryDrainStategy = powerlimiter["battery_drain_strategy"] | POWERLIMITER_BATTERY_DRAIN_STRATEGY;
    config.PowerLimiter.Interval =  powerlimiter["interval"] | POWERLIMITER_INTERVAL;
    config.PowerLimiter.IsInverterBehindPowerMeter = powerlimiter["is_inverter_behind_powermeter"] | POWERLIMITER_IS_INVERTER_BEHIND_POWER_METER;
    config.PowerLimiter.InverterId = powerlimiter["inverter_id"] | POWERLIMITER_INVERTER_ID;
    config.PowerLimiter.InverterChannelId = powerlimiter["inverter_channel_id"] | POWERLIMITER_INVERTER_CHANNEL_ID;
    config.PowerLimiter.TargetPowerConsumption = powerlimiter["target_power_consumption"] | POWERLIMITER_TARGET_POWER_CONSUMPTION;
    config.PowerLimiter.TargetPowerConsumptionHysteresis = powerlimiter["target_power_consumption_hysteresis"] | POWERLIMITER_TARGET_POWER_CONSUMPTION_HYSTERESIS;
    config.PowerLimiter.LowerPowerLimit = powerlimiter["lower_power_limit"] | POWERLIMITER_LOWER_POWER_LIMIT;
    config.PowerLimiter.UpperPowerLimit = powerlimiter["upper_power_limit"] | POWERLIMITER_UPPER_POWER_LIMIT;
    config.PowerLimiter.BatterySocStartThreshold = powerlimiter["battery_soc_start_threshold"] | POWERLIMITER_BATTERY_SOC_START_THRESHOLD;
    config.PowerLimiter.BatterySocStopThreshold = powerlimiter["battery_soc_stop_threshold"] | POWERLIMITER_BATTERY_SOC_STOP_THRESHOLD;
    config.PowerLimiter.VoltageStartThreshold = powerlimiter["voltage_start_threshold"] | POWERLIMITER_VOLTAGE_START_THRESHOLD;
    config.PowerLimiter.VoltageStopThreshold = powerlimiter["voltage_stop_threshold"] | POWERLIMITER_VOLTAGE_STOP_THRESHOLD;
    config.PowerLimiter.VoltageLoadCorrectionFactor = powerlimiter["voltage_load_correction_factor"] | POWERLIMITER_VOLTAGE_LOAD_CORRECTION_FACTOR;
    config.PowerLimiter.RestartHour = powerlimiter["inverter_restart_hour"] | POWERLIMITER_RESTART_HOUR;
    config.PowerLimiter.FullSolarPassThroughSoc = powerlimiter["full_solar_passthrough_soc"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_SOC;
    config.PowerLimiter.FullSolarPassThroughStartVoltage = powerlimiter["full_solar_passthrough_start_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_START_VOLTAGE;
    config.PowerLimiter.FullSolarPassThroughStopVoltage = powerlimiter["full_solar_passthrough_stop_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_STOP_VOLTAGE;

    JsonObject battery = doc["battery"];
    config.Battery.Enabled = battery["enabled"] | BATTERY_ENABLED;
    config.Battery.VerboseLogging = battery["verbose_logging"] | VERBOSE_LOGGING;
    config.Battery.Provider = battery["provider"] | BATTERY_PROVIDER;
    config.Battery.JkBmsInterface = battery["jkbms_interface"] | BATTERY_JKBMS_INTERFACE;
    config.Battery.JkBmsPollingInterval = battery["jkbms_polling_interval"] | BATTERY_JKBMS_POLLING_INTERVAL;
    strlcpy(config.Battery.MqttTopic, battery["mqtt_topic"] | "", sizeof(config.Battery.MqttTopic));

    JsonObject huawei = doc["huawei"];
    config.Huawei.Enabled = huawei["enabled"] | HUAWEI_ENABLED;
    config.Huawei.CAN_Controller_Frequency = huawei["can_controller_frequency"] | HUAWEI_CAN_CONTROLLER_FREQUENCY;
    config.Huawei.Auto_Power_Enabled = huawei["auto_power_enabled"] | false;
    config.Huawei.Auto_Power_Voltage_Limit = huawei["voltage_limit"] | HUAWEI_AUTO_POWER_VOLTAGE_LIMIT;
    config.Huawei.Auto_Power_Enable_Voltage_Limit =  huawei["enable_voltage_limit"] | HUAWEI_AUTO_POWER_ENABLE_VOLTAGE_LIMIT;
    config.Huawei.Auto_Power_Lower_Power_Limit = huawei["lower_power_limit"] | HUAWEI_AUTO_POWER_LOWER_POWER_LIMIT;
    config.Huawei.Auto_Power_Upper_Power_Limit = huawei["upper_power_limit"] | HUAWEI_AUTO_POWER_UPPER_POWER_LIMIT;

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

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.printf("Failed to read file, cancel migration: %s\r\n", error.c_str());
        return;
    }

    if (config.Cfg.Version < 0x00011700) {
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

    if (config.Cfg.Version < 0x00011800) {
        JsonObject mqtt = doc["mqtt"];
        config.Mqtt.PublishInterval = mqtt["publish_invterval"];
    }

    if (config.Cfg.Version < 0x00011900) {
        JsonObject dtu = doc["dtu"];
        config.Dtu.Nrf.PaLevel = dtu["pa_level"];
    }

    if (config.Cfg.Version < 0x00011a00) {
        // This migration fixes this issue: https://github.com/espressif/arduino-esp32/issues/8828
        // It occours when migrating from Core 2.0.9 to 2.0.14
        // which was done by updating ESP32 PlatformIO from 6.3.2 to 6.5.0
        nvs_flash_erase();
        nvs_flash_init();
    }

    f.close();

    config.Cfg.Version = CONFIG_VERSION;
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

    return nullptr;
}

INVERTER_CONFIG_T* ConfigurationClass::getInverterConfig(const uint64_t serial)
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == serial) {
            return &config.Inverter[i];
        }
    }

    return nullptr;
}

ConfigurationClass Configuration;

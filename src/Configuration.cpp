// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "Configuration.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "defaults.h"
#include <LittleFS.h>
#include <nvs_flash.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
}

void ConfigurationClass::serializeHttpRequestConfig(HttpRequestConfig const& source, JsonObject& target)
{
    JsonObject target_http_config = target["http_request"].to<JsonObject>();
    target_http_config["url"] = source.Url;
    target_http_config["auth_type"] = source.AuthType;
    target_http_config["username"] = source.Username;
    target_http_config["password"] = source.Password;
    target_http_config["header_key"] = source.HeaderKey;
    target_http_config["header_value"] = source.HeaderValue;
    target_http_config["timeout"] = source.Timeout;
}

void ConfigurationClass::serializePowerMeterMqttConfig(PowerMeterMqttConfig const& source, JsonObject& target)
{
    JsonArray values = target["values"].to<JsonArray>();
    for (size_t i = 0; i < POWERMETER_MQTT_MAX_VALUES; ++i) {
        JsonObject t = values.add<JsonObject>();
        PowerMeterMqttValue const& s = source.Values[i];

        t["topic"] = s.Topic;
        t["json_path"] = s.JsonPath;
        t["unit"] = s.PowerUnit;
        t["sign_inverted"] = s.SignInverted;
    }
}

void ConfigurationClass::serializePowerMeterSerialSdmConfig(PowerMeterSerialSdmConfig const& source, JsonObject& target)
{
    target["address"] = source.Address;
    target["polling_interval"] = source.PollingInterval;
}

void ConfigurationClass::serializePowerMeterHttpJsonConfig(PowerMeterHttpJsonConfig const& source, JsonObject& target)
{
    target["polling_interval"] = source.PollingInterval;
    target["individual_requests"] = source.IndividualRequests;

    JsonArray values = target["values"].to<JsonArray>();
    for (size_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; ++i) {
        JsonObject t = values.add<JsonObject>();
        PowerMeterHttpJsonValue const& s = source.Values[i];

        serializeHttpRequestConfig(s.HttpRequest, t);

        t["enabled"] = s.Enabled;
        t["json_path"] = s.JsonPath;
        t["unit"] = s.PowerUnit;
        t["sign_inverted"] = s.SignInverted;
    }
}

void ConfigurationClass::serializePowerMeterHttpSmlConfig(PowerMeterHttpSmlConfig const& source, JsonObject& target)
{
    target["polling_interval"] = source.PollingInterval;
    serializeHttpRequestConfig(source.HttpRequest, target);
}

void ConfigurationClass::serializeBatteryConfig(BatteryConfig const& source, JsonObject& target)
{
    target["enabled"] = config.Battery.Enabled;
    target["verbose_logging"] = config.Battery.VerboseLogging;
    target["provider"] = config.Battery.Provider;
    target["jkbms_interface"] = config.Battery.JkBmsInterface;
    target["jkbms_polling_interval"] = config.Battery.JkBmsPollingInterval;
    target["mqtt_soc_topic"] = config.Battery.MqttSocTopic;
    target["mqtt_soc_json_path"] = config.Battery.MqttSocJsonPath;
    target["mqtt_voltage_topic"] = config.Battery.MqttVoltageTopic;
    target["mqtt_voltage_json_path"] = config.Battery.MqttVoltageJsonPath;
    target["mqtt_voltage_unit"] = config.Battery.MqttVoltageUnit;
    target["enable_discharge_current_limit"] = config.Battery.EnableDischargeCurrentLimit;
    target["discharge_current_limit"] = config.Battery.DischargeCurrentLimit;
    target["discharge_current_limit_below_soc"] = config.Battery.DischargeCurrentLimitBelowSoc;
    target["discharge_current_limit_below_voltage"] = config.Battery.DischargeCurrentLimitBelowVoltage;
    target["use_battery_reported_discharge_current_limit"] = config.Battery.UseBatteryReportedDischargeCurrentLimit;
    target["mqtt_discharge_current_topic"] = config.Battery.MqttDischargeCurrentTopic;
    target["mqtt_discharge_current_json_path"] = config.Battery.MqttDischargeCurrentJsonPath;
    target["mqtt_amperage_unit"] = config.Battery.MqttAmperageUnit;
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg.SaveCount++;

    JsonDocument doc;

    JsonObject cfg = doc["cfg"].to<JsonObject>();
    cfg["version"] = config.Cfg.Version;
    cfg["save_count"] = config.Cfg.SaveCount;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
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

    JsonObject mdns = doc["mdns"].to<JsonObject>();
    mdns["enabled"] = config.Mdns.Enabled;

    JsonObject syslog = doc["syslog"].to<JsonObject>();
    syslog["enabled"] = config.Syslog.Enabled;
    syslog["hostname"] = config.Syslog.Hostname;
    syslog["port"] = config.Syslog.Port;

    JsonObject ntp = doc["ntp"].to<JsonObject>();
    ntp["server"] = config.Ntp.Server;
    ntp["timezone"] = config.Ntp.Timezone;
    ntp["timezone_descr"] = config.Ntp.TimezoneDescr;
    ntp["latitude"] = config.Ntp.Latitude;
    ntp["longitude"] = config.Ntp.Longitude;
    ntp["sunsettype"] = config.Ntp.SunsetType;

    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    mqtt["enabled"] = config.Mqtt.Enabled;
    mqtt["verbose_logging"] = config.Mqtt.VerboseLogging;
    mqtt["hostname"] = config.Mqtt.Hostname;
    mqtt["port"] = config.Mqtt.Port;
    mqtt["clientid"] = config.Mqtt.ClientId;
    mqtt["username"] = config.Mqtt.Username;
    mqtt["password"] = config.Mqtt.Password;
    mqtt["topic"] = config.Mqtt.Topic;
    mqtt["retain"] = config.Mqtt.Retain;
    mqtt["publish_interval"] = config.Mqtt.PublishInterval;
    mqtt["clean_session"] = config.Mqtt.CleanSession;

    JsonObject mqtt_lwt = mqtt["lwt"].to<JsonObject>();
    mqtt_lwt["topic"] = config.Mqtt.Lwt.Topic;
    mqtt_lwt["value_online"] = config.Mqtt.Lwt.Value_Online;
    mqtt_lwt["value_offline"] = config.Mqtt.Lwt.Value_Offline;
    mqtt_lwt["qos"] = config.Mqtt.Lwt.Qos;

    JsonObject mqtt_tls = mqtt["tls"].to<JsonObject>();
    mqtt_tls["enabled"] = config.Mqtt.Tls.Enabled;
    mqtt_tls["root_ca_cert"] = config.Mqtt.Tls.RootCaCert;
    mqtt_tls["certlogin"] = config.Mqtt.Tls.CertLogin;
    mqtt_tls["client_cert"] = config.Mqtt.Tls.ClientCert;
    mqtt_tls["client_key"] = config.Mqtt.Tls.ClientKey;

    JsonObject mqtt_hass = mqtt["hass"].to<JsonObject>();
    mqtt_hass["enabled"] = config.Mqtt.Hass.Enabled;
    mqtt_hass["retain"] = config.Mqtt.Hass.Retain;
    mqtt_hass["topic"] = config.Mqtt.Hass.Topic;
    mqtt_hass["individual_panels"] = config.Mqtt.Hass.IndividualPanels;
    mqtt_hass["expire"] = config.Mqtt.Hass.Expire;

    JsonObject dtu = doc["dtu"].to<JsonObject>();
    dtu["serial"] = config.Dtu.Serial;
    dtu["poll_interval"] = config.Dtu.PollInterval;
    dtu["verbose_logging"] = config.Dtu.VerboseLogging;
    dtu["nrf_pa_level"] = config.Dtu.Nrf.PaLevel;
    dtu["cmt_pa_level"] = config.Dtu.Cmt.PaLevel;
    dtu["cmt_frequency"] = config.Dtu.Cmt.Frequency;
    dtu["cmt_country_mode"] = config.Dtu.Cmt.CountryMode;

    JsonObject security = doc["security"].to<JsonObject>();
    security["password"] = config.Security.Password;
    security["allow_readonly"] = config.Security.AllowReadonly;

    JsonObject device = doc["device"].to<JsonObject>();
    device["pinmapping"] = config.Dev_PinMapping;

    JsonObject display = device["display"].to<JsonObject>();
    display["powersafe"] = config.Display.PowerSafe;
    display["screensaver"] = config.Display.ScreenSaver;
    display["rotation"] = config.Display.Rotation;
    display["contrast"] = config.Display.Contrast;
    display["language"] = config.Display.Language;
    display["diagram_duration"] = config.Display.Diagram.Duration;
    display["diagram_mode"] = config.Display.Diagram.Mode;

    JsonArray leds = device["led"].to<JsonArray>();
    for (uint8_t i = 0; i < PINMAPPING_LED_COUNT; i++) {
        JsonObject led = leds.add<JsonObject>();
        led["brightness"] = config.Led_Single[i].Brightness;
    }

    JsonArray inverters = doc["inverters"].to<JsonArray>();
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        JsonObject inv = inverters.add<JsonObject>();
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
        inv["clear_eventlog"] = config.Inverter[i].ClearEventlogOnMidnight;
        inv["yieldday_correction"] = config.Inverter[i].YieldDayCorrection;

        JsonArray channel = inv["channel"].to<JsonArray>();
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            JsonObject chanData = channel.add<JsonObject>();
            chanData["name"] = config.Inverter[i].channel[c].Name;
            chanData["max_power"] = config.Inverter[i].channel[c].MaxChannelPower;
            chanData["yield_total_offset"] = config.Inverter[i].channel[c].YieldTotalOffset;
        }
    }

    JsonObject vedirect = doc["vedirect"].to<JsonObject>();
    vedirect["enabled"] = config.Vedirect.Enabled;
    vedirect["verbose_logging"] = config.Vedirect.VerboseLogging;
    vedirect["updates_only"] = config.Vedirect.UpdatesOnly;

    JsonObject powermeter = doc["powermeter"].to<JsonObject>();
    powermeter["enabled"] = config.PowerMeter.Enabled;
    powermeter["verbose_logging"] = config.PowerMeter.VerboseLogging;
    powermeter["source"] = config.PowerMeter.Source;

    JsonObject powermeter_mqtt = powermeter["mqtt"].to<JsonObject>();
    serializePowerMeterMqttConfig(config.PowerMeter.Mqtt, powermeter_mqtt);

    JsonObject powermeter_serial_sdm = powermeter["serial_sdm"].to<JsonObject>();
    serializePowerMeterSerialSdmConfig(config.PowerMeter.SerialSdm, powermeter_serial_sdm);

    JsonObject powermeter_http_json = powermeter["http_json"].to<JsonObject>();
    serializePowerMeterHttpJsonConfig(config.PowerMeter.HttpJson, powermeter_http_json);

    JsonObject powermeter_http_sml = powermeter["http_sml"].to<JsonObject>();
    serializePowerMeterHttpSmlConfig(config.PowerMeter.HttpSml, powermeter_http_sml);

    JsonObject powerlimiter = doc["powerlimiter"].to<JsonObject>();
    powerlimiter["enabled"] = config.PowerLimiter.Enabled;
    powerlimiter["verbose_logging"] = config.PowerLimiter.VerboseLogging;
    powerlimiter["solar_passtrough_enabled"] = config.PowerLimiter.SolarPassThroughEnabled;
    powerlimiter["solar_passthrough_losses"] = config.PowerLimiter.SolarPassThroughLosses;
    powerlimiter["battery_always_use_at_night"] = config.PowerLimiter.BatteryAlwaysUseAtNight;
    powerlimiter["interval"] = config.PowerLimiter.Interval;
    powerlimiter["is_inverter_behind_powermeter"] = config.PowerLimiter.IsInverterBehindPowerMeter;
    powerlimiter["is_inverter_solar_powered"] = config.PowerLimiter.IsInverterSolarPowered;
    powerlimiter["use_overscaling_to_compensate_shading"] = config.PowerLimiter.UseOverscalingToCompensateShading;
    powerlimiter["inverter_id"] = config.PowerLimiter.InverterId;
    powerlimiter["inverter_channel_id"] = config.PowerLimiter.InverterChannelId;
    powerlimiter["target_power_consumption"] = config.PowerLimiter.TargetPowerConsumption;
    powerlimiter["target_power_consumption_hysteresis"] = config.PowerLimiter.TargetPowerConsumptionHysteresis;
    powerlimiter["lower_power_limit"] = config.PowerLimiter.LowerPowerLimit;
    powerlimiter["base_load_limit"] = config.PowerLimiter.BaseLoadLimit;
    powerlimiter["upper_power_limit"] = config.PowerLimiter.UpperPowerLimit;
    powerlimiter["ignore_soc"] = config.PowerLimiter.IgnoreSoc;
    powerlimiter["battery_soc_start_threshold"] = config.PowerLimiter.BatterySocStartThreshold;
    powerlimiter["battery_soc_stop_threshold"] = config.PowerLimiter.BatterySocStopThreshold;
    powerlimiter["voltage_start_threshold"] = config.PowerLimiter.VoltageStartThreshold;
    powerlimiter["voltage_stop_threshold"] = config.PowerLimiter.VoltageStopThreshold;
    powerlimiter["voltage_load_correction_factor"] = config.PowerLimiter.VoltageLoadCorrectionFactor;
    powerlimiter["inverter_restart_hour"] = config.PowerLimiter.RestartHour;
    powerlimiter["full_solar_passthrough_soc"] = config.PowerLimiter.FullSolarPassThroughSoc;
    powerlimiter["full_solar_passthrough_start_voltage"] = config.PowerLimiter.FullSolarPassThroughStartVoltage;
    powerlimiter["full_solar_passthrough_stop_voltage"] = config.PowerLimiter.FullSolarPassThroughStopVoltage;

    JsonObject battery = doc["battery"].to<JsonObject>();
    serializeBatteryConfig(config.Battery, battery);

    JsonObject huawei = doc["huawei"].to<JsonObject>();
    huawei["enabled"] = config.Huawei.Enabled;
    huawei["verbose_logging"] = config.Huawei.VerboseLogging;
    huawei["can_controller_frequency"] = config.Huawei.CAN_Controller_Frequency;
    huawei["auto_power_enabled"] = config.Huawei.Auto_Power_Enabled;
    huawei["auto_power_batterysoc_limits_enabled"] = config.Huawei.Auto_Power_BatterySoC_Limits_Enabled;
    huawei["emergency_charge_enabled"] = config.Huawei.Emergency_Charge_Enabled;
    huawei["voltage_limit"] = config.Huawei.Auto_Power_Voltage_Limit;
    huawei["enable_voltage_limit"] = config.Huawei.Auto_Power_Enable_Voltage_Limit;
    huawei["lower_power_limit"] = config.Huawei.Auto_Power_Lower_Power_Limit;
    huawei["upper_power_limit"] = config.Huawei.Auto_Power_Upper_Power_Limit;
    huawei["stop_batterysoc_threshold"] = config.Huawei.Auto_Power_Stop_BatterySoC_Threshold;
    huawei["target_power_consumption"] = config.Huawei.Auto_Power_Target_Power_Consumption;

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
    }

    // Serialize JSON to file
    if (serializeJson(doc, f) == 0) {
        MessageOutput.println("Failed to write file");
        return false;
    }

    f.close();
    return true;
}

void ConfigurationClass::deserializeHttpRequestConfig(JsonObject const& source, HttpRequestConfig& target)
{
    JsonObject source_http_config = source["http_request"];

    // http request parameters of HTTP/JSON power meter were previously stored
    // alongside other settings. TODO(schlimmchen): remove in early 2025.
    if (source_http_config.isNull()) { source_http_config = source; }

    strlcpy(target.Url, source_http_config["url"] | "", sizeof(target.Url));
    target.AuthType = source_http_config["auth_type"] | HttpRequestConfig::Auth::None;
    strlcpy(target.Username, source_http_config["username"] | "", sizeof(target.Username));
    strlcpy(target.Password, source_http_config["password"] | "", sizeof(target.Password));
    strlcpy(target.HeaderKey, source_http_config["header_key"] | "", sizeof(target.HeaderKey));
    strlcpy(target.HeaderValue, source_http_config["header_value"] | "", sizeof(target.HeaderValue));
    target.Timeout = source_http_config["timeout"] | HTTP_REQUEST_TIMEOUT_MS;
}

void ConfigurationClass::deserializePowerMeterMqttConfig(JsonObject const& source, PowerMeterMqttConfig& target)
{
    for (size_t i = 0; i < POWERMETER_MQTT_MAX_VALUES; ++i) {
        PowerMeterMqttValue& t = target.Values[i];
        JsonObject s = source["values"][i];

        strlcpy(t.Topic, s["topic"] | "", sizeof(t.Topic));
        strlcpy(t.JsonPath, s["json_path"] | "", sizeof(t.JsonPath));
        t.PowerUnit = s["unit"] | PowerMeterMqttValue::Unit::Watts;
        t.SignInverted = s["sign_inverted"] | false;
    }
}

void ConfigurationClass::deserializePowerMeterSerialSdmConfig(JsonObject const& source, PowerMeterSerialSdmConfig& target)
{
    target.PollingInterval = source["polling_interval"] | POWERMETER_POLLING_INTERVAL;
    target.Address = source["address"] | POWERMETER_SDMADDRESS;
}

void ConfigurationClass::deserializePowerMeterHttpJsonConfig(JsonObject const& source, PowerMeterHttpJsonConfig& target)
{
    target.PollingInterval = source["polling_interval"] | POWERMETER_POLLING_INTERVAL;
    target.IndividualRequests = source["individual_requests"] | false;

    JsonArray values = source["values"].as<JsonArray>();
    for (size_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; ++i) {
        PowerMeterHttpJsonValue& t = target.Values[i];
        JsonObject s = values[i];

        deserializeHttpRequestConfig(s, t.HttpRequest);

        t.Enabled = s["enabled"] | false;
        strlcpy(t.JsonPath, s["json_path"] | "", sizeof(t.JsonPath));
        t.PowerUnit = s["unit"] | PowerMeterHttpJsonValue::Unit::Watts;
        t.SignInverted = s["sign_inverted"] | false;
    }

    target.Values[0].Enabled = true;
}

void ConfigurationClass::deserializePowerMeterHttpSmlConfig(JsonObject const& source, PowerMeterHttpSmlConfig& target)
{
    target.PollingInterval = source["polling_interval"] | POWERMETER_POLLING_INTERVAL;
    deserializeHttpRequestConfig(source, target.HttpRequest);
}

void ConfigurationClass::deserializeBatteryConfig(JsonObject const& source, BatteryConfig& target)
{
    target.Enabled = source["enabled"] | BATTERY_ENABLED;
    target.VerboseLogging = source["verbose_logging"] | VERBOSE_LOGGING;
    target.Provider = source["provider"] | BATTERY_PROVIDER;
    target.JkBmsInterface = source["jkbms_interface"] | BATTERY_JKBMS_INTERFACE;
    target.JkBmsPollingInterval = source["jkbms_polling_interval"] | BATTERY_JKBMS_POLLING_INTERVAL;
    strlcpy(target.MqttSocTopic, source["mqtt_soc_topic"] | source["mqtt_topic"] | "", sizeof(config.Battery.MqttSocTopic)); // mqtt_soc_topic was previously saved as mqtt_topic. Be nice and also try old key.
    strlcpy(target.MqttSocJsonPath, source["mqtt_soc_json_path"] | source["mqtt_json_path"] | "", sizeof(config.Battery.MqttSocJsonPath)); // mqtt_soc_json_path was previously saved as mqtt_json_path. Be nice and also try old key.
    strlcpy(target.MqttVoltageTopic, source["mqtt_voltage_topic"] | "", sizeof(config.Battery.MqttVoltageTopic));
    strlcpy(target.MqttVoltageJsonPath, source["mqtt_voltage_json_path"] | "", sizeof(config.Battery.MqttVoltageJsonPath));
    target.MqttVoltageUnit = source["mqtt_voltage_unit"] | BatteryVoltageUnit::Volts;
    target.EnableDischargeCurrentLimit = source["enable_discharge_current_limit"] | BATTERY_ENABLE_DISCHARGE_CURRENT_LIMIT;
    target.DischargeCurrentLimit = source["discharge_current_limit"] | BATTERY_DISCHARGE_CURRENT_LIMIT;
    target.DischargeCurrentLimitBelowSoc = source["discharge_current_limit_below_soc"] | BATTERY_DISCHARGE_CURRENT_LIMIT_BELOW_SOC;
    target.DischargeCurrentLimitBelowVoltage = source["discharge_current_limit_below_voltage"] | BATTERY_DISCHARGE_CURRENT_LIMIT_BELOW_VOLTAGE;
    target.UseBatteryReportedDischargeCurrentLimit = source["use_battery_reported_discharge_current_limit"] | BATTERY_USE_BATTERY_REPORTED_DISCHARGE_CURRENT_LIMIT;
    strlcpy(target.MqttDischargeCurrentTopic, source["mqtt_discharge_current_topic"] | "", sizeof(config.Battery.MqttDischargeCurrentTopic));
    strlcpy(target.MqttDischargeCurrentJsonPath, source["mqtt_discharge_current_json_path"] | "", sizeof(config.Battery.MqttDischargeCurrentJsonPath));
    target.MqttAmperageUnit = source["mqtt_amperage_unit"] | BatteryAmperageUnit::Amps;
}

bool ConfigurationClass::read()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r", false);

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return false;
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

    JsonObject syslog = doc["syslog"];
    config.Syslog.Enabled = syslog["enabled"] | SYSLOG_ENABLED;
    strlcpy(config.Syslog.Hostname, syslog["hostname"] | "", sizeof(config.Syslog.Hostname));
    config.Syslog.Port = syslog["port"] | SYSLOG_PORT;

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
    strlcpy(config.Mqtt.ClientId, mqtt["clientid"] | NetworkSettings.getApName().c_str(), sizeof(config.Mqtt.ClientId));
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
    config.Dtu.Cmt.CountryMode = dtu["cmt_country_mode"] | DTU_CMT_COUNTRY_MODE;

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
    config.Display.Diagram.Duration = display["diagram_duration"] | DISPLAY_DIAGRAM_DURATION;
    config.Display.Diagram.Mode = display["diagram_mode"] | DISPLAY_DIAGRAM_MODE;

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
        config.Inverter[i].ClearEventlogOnMidnight = inv["clear_eventlog"] | false;
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
    config.PowerMeter.Source =  powermeter["source"] | POWERMETER_SOURCE;

    deserializePowerMeterMqttConfig(powermeter["mqtt"], config.PowerMeter.Mqtt);

    // process settings from legacy config if they are present
    // TODO(schlimmchen): remove in early 2025.
    if (!powermeter["mqtt_topic_powermeter_1"].isNull()) {
        auto& values = config.PowerMeter.Mqtt.Values;
        strlcpy(values[0].Topic, powermeter["mqtt_topic_powermeter_1"], sizeof(values[0].Topic));
        strlcpy(values[1].Topic, powermeter["mqtt_topic_powermeter_2"], sizeof(values[1].Topic));
        strlcpy(values[2].Topic, powermeter["mqtt_topic_powermeter_3"], sizeof(values[2].Topic));
    }

    deserializePowerMeterSerialSdmConfig(powermeter["serial_sdm"], config.PowerMeter.SerialSdm);

    // process settings from legacy config if they are present
    // TODO(schlimmchen): remove in early 2025.
    if (!powermeter["sdmaddress"].isNull()) {
        config.PowerMeter.SerialSdm.Address = powermeter["sdmaddress"];
    }

    JsonObject powermeter_http_json = powermeter["http_json"];
    deserializePowerMeterHttpJsonConfig(powermeter_http_json, config.PowerMeter.HttpJson);

    JsonObject powermeter_sml = powermeter["http_sml"];
    deserializePowerMeterHttpSmlConfig(powermeter_sml, config.PowerMeter.HttpSml);

    // process settings from legacy config if they are present
    // TODO(schlimmchen): remove in early 2025.
    if (!powermeter["http_phases"].isNull()) {
        auto& target = config.PowerMeter.HttpJson;

        for (size_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; ++i) {
            PowerMeterHttpJsonValue& t = target.Values[i];
            JsonObject s = powermeter["http_phases"][i];

            deserializeHttpRequestConfig(s, t.HttpRequest);

            t.Enabled = s["enabled"] | false;
            strlcpy(t.JsonPath, s["json_path"] | "", sizeof(t.JsonPath));
            t.PowerUnit = s["unit"] | PowerMeterHttpJsonValue::Unit::Watts;
            t.SignInverted = s["sign_inverted"] | false;
        }

        target.IndividualRequests = powermeter["http_individual_requests"] | false;
    }

    JsonObject powerlimiter = doc["powerlimiter"];
    config.PowerLimiter.Enabled = powerlimiter["enabled"] | POWERLIMITER_ENABLED;
    config.PowerLimiter.VerboseLogging = powerlimiter["verbose_logging"] | VERBOSE_LOGGING;
    config.PowerLimiter.SolarPassThroughEnabled = powerlimiter["solar_passtrough_enabled"] | POWERLIMITER_SOLAR_PASSTHROUGH_ENABLED;
    config.PowerLimiter.SolarPassThroughLosses = powerlimiter["solar_passthrough_losses"] | powerlimiter["solar_passtrough_losses"] | POWERLIMITER_SOLAR_PASSTHROUGH_LOSSES; // solar_passthrough_losses was previously saved as solar_passtrough_losses. Be nice and also try mistyped key.
    config.PowerLimiter.BatteryAlwaysUseAtNight = powerlimiter["battery_always_use_at_night"] | POWERLIMITER_BATTERY_ALWAYS_USE_AT_NIGHT;
    if (powerlimiter["battery_drain_strategy"].as<uint8_t>() == 1) { config.PowerLimiter.BatteryAlwaysUseAtNight = true; } // convert legacy setting
    config.PowerLimiter.Interval =  powerlimiter["interval"] | POWERLIMITER_INTERVAL;
    config.PowerLimiter.IsInverterBehindPowerMeter = powerlimiter["is_inverter_behind_powermeter"] | POWERLIMITER_IS_INVERTER_BEHIND_POWER_METER;
    config.PowerLimiter.IsInverterSolarPowered = powerlimiter["is_inverter_solar_powered"] | POWERLIMITER_IS_INVERTER_SOLAR_POWERED;
    config.PowerLimiter.UseOverscalingToCompensateShading = powerlimiter["use_overscaling_to_compensate_shading"] | POWERLIMITER_USE_OVERSCALING_TO_COMPENSATE_SHADING;
    config.PowerLimiter.InverterId = powerlimiter["inverter_id"] | POWERLIMITER_INVERTER_ID;
    config.PowerLimiter.InverterChannelId = powerlimiter["inverter_channel_id"] | POWERLIMITER_INVERTER_CHANNEL_ID;
    config.PowerLimiter.TargetPowerConsumption = powerlimiter["target_power_consumption"] | POWERLIMITER_TARGET_POWER_CONSUMPTION;
    config.PowerLimiter.TargetPowerConsumptionHysteresis = powerlimiter["target_power_consumption_hysteresis"] | POWERLIMITER_TARGET_POWER_CONSUMPTION_HYSTERESIS;
    config.PowerLimiter.LowerPowerLimit = powerlimiter["lower_power_limit"] | POWERLIMITER_LOWER_POWER_LIMIT;
    config.PowerLimiter.BaseLoadLimit = powerlimiter["base_load_limit"] | POWERLIMITER_BASE_LOAD_LIMIT;
    config.PowerLimiter.UpperPowerLimit = powerlimiter["upper_power_limit"] | POWERLIMITER_UPPER_POWER_LIMIT;
    config.PowerLimiter.IgnoreSoc = powerlimiter["ignore_soc"] | POWERLIMITER_IGNORE_SOC;
    config.PowerLimiter.BatterySocStartThreshold = powerlimiter["battery_soc_start_threshold"] | POWERLIMITER_BATTERY_SOC_START_THRESHOLD;
    config.PowerLimiter.BatterySocStopThreshold = powerlimiter["battery_soc_stop_threshold"] | POWERLIMITER_BATTERY_SOC_STOP_THRESHOLD;
    config.PowerLimiter.VoltageStartThreshold = powerlimiter["voltage_start_threshold"] | POWERLIMITER_VOLTAGE_START_THRESHOLD;
    config.PowerLimiter.VoltageStopThreshold = powerlimiter["voltage_stop_threshold"] | POWERLIMITER_VOLTAGE_STOP_THRESHOLD;
    config.PowerLimiter.VoltageLoadCorrectionFactor = powerlimiter["voltage_load_correction_factor"] | POWERLIMITER_VOLTAGE_LOAD_CORRECTION_FACTOR;
    config.PowerLimiter.RestartHour = powerlimiter["inverter_restart_hour"] | POWERLIMITER_RESTART_HOUR;
    config.PowerLimiter.FullSolarPassThroughSoc = powerlimiter["full_solar_passthrough_soc"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_SOC;
    config.PowerLimiter.FullSolarPassThroughStartVoltage = powerlimiter["full_solar_passthrough_start_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_START_VOLTAGE;
    config.PowerLimiter.FullSolarPassThroughStopVoltage = powerlimiter["full_solar_passthrough_stop_voltage"] | POWERLIMITER_FULL_SOLAR_PASSTHROUGH_STOP_VOLTAGE;

    deserializeBatteryConfig(doc["battery"], config.Battery);

    JsonObject huawei = doc["huawei"];
    config.Huawei.Enabled = huawei["enabled"] | HUAWEI_ENABLED;
    config.Huawei.VerboseLogging = huawei["verbose_logging"] | VERBOSE_LOGGING;
    config.Huawei.CAN_Controller_Frequency = huawei["can_controller_frequency"] | HUAWEI_CAN_CONTROLLER_FREQUENCY;
    config.Huawei.Auto_Power_Enabled = huawei["auto_power_enabled"] | false;
    config.Huawei.Auto_Power_BatterySoC_Limits_Enabled = huawei["auto_power_batterysoc_limits_enabled"] | false;
    config.Huawei.Emergency_Charge_Enabled = huawei["emergency_charge_enabled"] | false;
    config.Huawei.Auto_Power_Voltage_Limit = huawei["voltage_limit"] | HUAWEI_AUTO_POWER_VOLTAGE_LIMIT;
    config.Huawei.Auto_Power_Enable_Voltage_Limit =  huawei["enable_voltage_limit"] | HUAWEI_AUTO_POWER_ENABLE_VOLTAGE_LIMIT;
    config.Huawei.Auto_Power_Lower_Power_Limit = huawei["lower_power_limit"] | HUAWEI_AUTO_POWER_LOWER_POWER_LIMIT;
    config.Huawei.Auto_Power_Upper_Power_Limit = huawei["upper_power_limit"] | HUAWEI_AUTO_POWER_UPPER_POWER_LIMIT;
    config.Huawei.Auto_Power_Stop_BatterySoC_Threshold = huawei["stop_batterysoc_threshold"] | HUAWEI_AUTO_POWER_STOP_BATTERYSOC_THRESHOLD;
    config.Huawei.Auto_Power_Target_Power_Consumption = huawei["target_power_consumption"] | HUAWEI_AUTO_POWER_TARGET_POWER_CONSUMPTION;

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

    JsonDocument doc;

    // Deserialize the JSON document
    const DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.printf("Failed to read file, cancel migration: %s\r\n", error.c_str());
        return;
    }

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
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

    if (config.Cfg.Version < 0x00011b00) {
        // Convert from kHz to Hz
        config.Dtu.Cmt.Frequency *= 1000;
    }

    if (config.Cfg.Version < 0x00011c00) {
        if (!strcmp(config.Ntp.Server, NTP_SERVER_OLD)) {
            strlcpy(config.Ntp.Server, NTP_SERVER, sizeof(config.Ntp.Server));
        }
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

void ConfigurationClass::deleteInverterById(const uint8_t id)
{
    config.Inverter[id].Serial = 0ULL;
    strlcpy(config.Inverter[id].Name, "", sizeof(config.Inverter[id].Name));
    config.Inverter[id].Order = 0;

    config.Inverter[id].Poll_Enable = true;
    config.Inverter[id].Poll_Enable_Night = true;
    config.Inverter[id].Command_Enable = true;
    config.Inverter[id].Command_Enable_Night = true;
    config.Inverter[id].ReachableThreshold = REACHABLE_THRESHOLD;
    config.Inverter[id].ZeroRuntimeDataIfUnrechable = false;
    config.Inverter[id].ZeroYieldDayOnMidnight = false;
    config.Inverter[id].YieldDayCorrection = false;

    for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
        config.Inverter[id].channel[c].MaxChannelPower = 0;
        config.Inverter[id].channel[c].YieldTotalOffset = 0.0f;
        strlcpy(config.Inverter[id].channel[c].Name, "", sizeof(config.Inverter[id].channel[c].Name));
    }
}

ConfigurationClass Configuration;

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "defaults.h"
#include <LittleFS.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
    config.Cfg_SaveCount = 0;
    config.Cfg_Version = CONFIG_VERSION;

    // WiFi Settings
    strlcpy(config.WiFi_Ssid, WIFI_SSID, sizeof(config.WiFi_Ssid));
    strlcpy(config.WiFi_Password, WIFI_PASSWORD, sizeof(config.WiFi_Password));
    config.WiFi_Dhcp = WIFI_DHCP;
    strlcpy(config.WiFi_Hostname, APP_HOSTNAME, sizeof(config.WiFi_Hostname));

    // NTP Settings
    strlcpy(config.Ntp_Server, NTP_SERVER, sizeof(config.Ntp_Server));
    strlcpy(config.Ntp_Timezone, NTP_TIMEZONE, sizeof(config.Ntp_Timezone));
    strlcpy(config.Ntp_TimezoneDescr, NTP_TIMEZONEDESCR, sizeof(config.Ntp_TimezoneDescr));

    // MqTT Settings
    config.Mqtt_Enabled = MQTT_ENABLED;
    strlcpy(config.Mqtt_Hostname, MQTT_HOST, sizeof(config.Mqtt_Hostname));
    config.Mqtt_Port = MQTT_PORT;
    strlcpy(config.Mqtt_Username, MQTT_USER, sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, MQTT_PASSWORD, sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, MQTT_TOPIC, sizeof(config.Mqtt_Topic));
    config.Mqtt_Retain = MQTT_RETAIN;
    config.Mqtt_Tls = MQTT_TLS;
    strlcpy(config.Mqtt_RootCaCert, MQTT_ROOT_CA_CERT, sizeof(config.Mqtt_RootCaCert));
    strlcpy(config.Mqtt_LwtTopic, MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));
    config.Mqtt_PublishInterval = MQTT_PUBLISH_INTERVAL;

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        config.Inverter[i].Serial = 0;
        strlcpy(config.Inverter[i].Name, "", 0);
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            config.Inverter[0].MaxChannelPower[c] = 0;
        }
    }

    config.Dtu_Serial = DTU_SERIAL;
    config.Dtu_PollInterval = DTU_POLL_INTERVAL;
    config.Dtu_PaLevel = DTU_PA_LEVEL;

    config.Mqtt_Hass_Enabled = MQTT_HASS_ENABLED;
    config.Mqtt_Hass_Expire = MQTT_HASS_EXPIRE;
    config.Mqtt_Hass_Retain = MQTT_HASS_RETAIN;
    strlcpy(config.Mqtt_Hass_Topic, MQTT_HASS_TOPIC, sizeof(config.Mqtt_Hass_Topic));
    config.Mqtt_Hass_IndividualPanels = MQTT_HASS_INDIVIDUALPANELS;
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&config);
    for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
        f.write(bytes[i]);
    }
    f.close();
    return true;
}

bool ConfigurationClass::read()
{
    File f = LittleFS.open(CONFIG_FILENAME, "r");
    if (!f) {
        return false;
    }
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&config);
    for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
        bytes[i] = f.read();
    }
    f.close();
    return true;
}

void ConfigurationClass::migrate()
{
    if (config.Cfg_Version < 0x00010400) {
        strlcpy(config.Ntp_Server, NTP_SERVER, sizeof(config.Ntp_Server));
        strlcpy(config.Ntp_Timezone, NTP_TIMEZONE, sizeof(config.Ntp_Timezone));
        strlcpy(config.Ntp_TimezoneDescr, NTP_TIMEZONEDESCR, sizeof(config.Ntp_TimezoneDescr));
    }

    if (config.Cfg_Version < 0x00010500) {
        config.Mqtt_Enabled = MQTT_ENABLED;
        strlcpy(config.Mqtt_Hostname, MQTT_HOST, sizeof(config.Mqtt_Hostname));
        config.Mqtt_Port = MQTT_PORT;
        strlcpy(config.Mqtt_Username, MQTT_USER, sizeof(config.Mqtt_Username));
        strlcpy(config.Mqtt_Password, MQTT_PASSWORD, sizeof(config.Mqtt_Password));
        strlcpy(config.Mqtt_Topic, MQTT_TOPIC, sizeof(config.Mqtt_Topic));
    }

    if (config.Cfg_Version < 0x00010600) {
        config.Mqtt_Retain = MQTT_RETAIN;
    }

    if (config.Cfg_Version < 0x00010700) {
        strlcpy(config.Mqtt_LwtTopic, MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
        strlcpy(config.Mqtt_LwtValue_Online, MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
        strlcpy(config.Mqtt_LwtValue_Offline, MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));
    }

    if (config.Cfg_Version < 0x00010800) {
        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            config.Inverter[i].Serial = 0;
            strlcpy(config.Inverter[i].Name, "", 0);
        }
    }

    if (config.Cfg_Version < 0x00010900) {
        config.Dtu_Serial = DTU_SERIAL;
        config.Dtu_PollInterval = DTU_POLL_INTERVAL;
        config.Dtu_PaLevel = DTU_PA_LEVEL;
    }

    if (config.Cfg_Version < 0x00011000) {
        config.Mqtt_PublishInterval = MQTT_PUBLISH_INTERVAL;
    }

    if (config.Cfg_Version < 0x00011100) {
        init(); // Config will be completly incompatible after this update
    }

    if (config.Cfg_Version < 0x00011200) {
        config.Mqtt_Hass_Enabled = MQTT_HASS_ENABLED;
        config.Mqtt_Hass_Retain = MQTT_HASS_RETAIN;
        strlcpy(config.Mqtt_Hass_Topic, MQTT_HASS_TOPIC, sizeof(config.Mqtt_Hass_Topic));
        config.Mqtt_Hass_IndividualPanels = MQTT_HASS_INDIVIDUALPANELS;
    }

    if (config.Cfg_Version < 0x00011300) {
        config.Mqtt_Tls = MQTT_TLS;
        strlcpy(config.Mqtt_RootCaCert, MQTT_ROOT_CA_CERT, sizeof(config.Mqtt_RootCaCert));
    }

    if (config.Cfg_Version < 0x00011400) {
        strlcpy(config.Mqtt_Hostname, config.Mqtt_Hostname_Short, sizeof(config.Mqtt_Hostname_Short));
    }

    if (config.Cfg_Version < 0x00011500) {
        config.Mqtt_Hass_Expire = MQTT_HASS_EXPIRE;
    }

    config.Cfg_Version = CONFIG_VERSION;
    write();
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

ConfigurationClass Configuration;
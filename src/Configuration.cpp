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
    strlcpy(config.Mqtt_LwtTopic, MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;
    uint8_t* bytes = (uint8_t*)&config;
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
    uint8_t* bytes = (uint8_t*)&config;
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

    config.Cfg_Version = CONFIG_VERSION;
    write();
}

CONFIG_T& ConfigurationClass::get()
{
    return config;
}

ConfigurationClass Configuration;
#pragma once

#define SERIAL_BAUDRATE 115200

#define APP_HOSTNAME "OpenDTU-%06X"

#define HTTP_PORT 80

#define ACCESS_POINT_NAME "OpenDTU-"
#define ACCESS_POINT_PASSWORD "openDTU42"

#define ADMIN_TIMEOUT 180
#define WIFI_RECONNECT_TIMEOUT 15
#define WIFI_RECONNECT_REDO_TIMEOUT 600

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define WIFI_DHCP true

#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_TIMEZONEDESCR "Europe/Berlin"

#define MQTT_ENABLED false
#define MQTT_HOST ""
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_TOPIC "stripe/"
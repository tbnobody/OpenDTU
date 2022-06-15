#include "WebApi.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "Hoymiles.h"
#include "MqttSettings.h"
#include "NtpSettings.h"
#include "Update.h"
#include "WiFiSettings.h"
#include "defaults.h"
#include "helper.h"
#include <LittleFS.h>
#include <ResetReason.h>

WebApiClass::WebApiClass()
    : _server(HTTP_PORT)
    , _ws("/ws")
    , _events("/events")
{
}

extern const uint8_t file_index_html_start[] asm("_binary_data_index_html_gz_start");
extern const uint8_t file_favicon_ico_start[] asm("_binary_data_favicon_ico_start");
extern const uint8_t file_zones_json_start[] asm("_binary_data_zones_json_gz_start");
extern const uint8_t file_app_js_start[] asm("_binary_data_js_app_js_gz_start");

extern const uint8_t file_index_html_end[] asm("_binary_data_index_html_gz_end");
extern const uint8_t file_favicon_ico_end[] asm("_binary_data_favicon_ico_end");
extern const uint8_t file_zones_json_end[] asm("_binary_data_zones_json_gz_end");
extern const uint8_t file_app_js_end[] asm("_binary_data_js_app_js_gz_end");

void WebApiClass::init()
{
    using namespace std::placeholders;

    _server.addHandler(&_ws);
    _server.addHandler(&_events);

    _ws.onEvent(std::bind(&WebApiClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    _server.on("/api/system/status", HTTP_GET, std::bind(&WebApiClass::onSystemStatus, this, _1));

    _server.on("/api/network/status", HTTP_GET, std::bind(&WebApiClass::onNetworkStatus, this, _1));
    _server.on("/api/network/config", HTTP_GET, std::bind(&WebApiClass::onNetworkAdminGet, this, _1));
    _server.on("/api/network/config", HTTP_POST, std::bind(&WebApiClass::onNetworkAdminPost, this, _1));

    _server.on("/api/ntp/status", HTTP_GET, std::bind(&WebApiClass::onNtpStatus, this, _1));
    _server.on("/api/ntp/config", HTTP_GET, std::bind(&WebApiClass::onNtpAdminGet, this, _1));
    _server.on("/api/ntp/config", HTTP_POST, std::bind(&WebApiClass::onNtpAdminPost, this, _1));

    _server.on("/api/mqtt/status", HTTP_GET, std::bind(&WebApiClass::onMqttStatus, this, _1));
    _server.on("/api/mqtt/config", HTTP_GET, std::bind(&WebApiClass::onMqttAdminGet, this, _1));
    _server.on("/api/mqtt/config", HTTP_POST, std::bind(&WebApiClass::onMqttAdminPost, this, _1));

    _server.on("/api/inverter/list", HTTP_GET, std::bind(&WebApiClass::onInverterList, this, _1));
    _server.on("/api/inverter/add", HTTP_POST, std::bind(&WebApiClass::onInverterAdd, this, _1));
    _server.on("/api/inverter/edit", HTTP_POST, std::bind(&WebApiClass::onInverterEdit, this, _1));
    _server.on("/api/inverter/del", HTTP_POST, std::bind(&WebApiClass::onInverterDelete, this, _1));

    _server.on("/api/dtu/config", HTTP_GET, std::bind(&WebApiClass::onDtuAdminGet, this, _1));
    _server.on("/api/dtu/config", HTTP_POST, std::bind(&WebApiClass::onDtuAdminPost, this, _1));

    _server.on("/api/firmware/update", HTTP_POST,
        std::bind(&WebApiClass::onFirmwareUpdateFinish, this, _1),
        std::bind(&WebApiClass::onFirmwareUpdateUpload, this, _1, _2, _3, _4, _5, _6));

    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", file_index_html_start, file_index_html_end - file_index_html_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", file_index_html_start, file_index_html_end - file_index_html_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "image/x-icon", file_favicon_ico_start, file_favicon_ico_end - file_favicon_ico_start);
        request->send(response);
    });

    _server.on("/zones.json", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "application/json", file_zones_json_start, file_zones_json_end - file_zones_json_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server.on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/javascript", file_app_js_start, file_app_js_end - file_app_js_start);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    _server.onNotFound(std::bind(&WebApiClass::onNotFound, this, _1));
    _server.begin();
}

void WebApiClass::loop()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

void WebApiClass::onNotFound(AsyncWebServerRequest* request)
{
    // Handle Unknown Request
    request->send(404, "text/plain", "404 Not Found");
}

void WebApiClass::onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        char str[64];
        sprintf(str, "Websocket: [%s][%u] connect", server->url(), client->id());
        Serial.println(str);
    } else if (type == WS_EVT_DISCONNECT) {
        char str[64];
        sprintf(str, "Websocket: [%s][%u] disconnect", server->url(), client->id());
        Serial.println(str);
    }
}

void WebApiClass::onNetworkStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root[F("sta_status")] = ((WiFi.getMode() & WIFI_STA) != 0);
    root[F("sta_ssid")] = WiFi.SSID();
    root[F("sta_ip")] = WiFi.localIP().toString();
    root[F("sta_netmask")] = WiFi.subnetMask().toString();
    root[F("sta_gateway")] = WiFi.gatewayIP().toString();
    root[F("sta_dns1")] = WiFi.dnsIP(0).toString();
    root[F("sta_dns2")] = WiFi.dnsIP(1).toString();
    root[F("sta_mac")] = WiFi.macAddress();
    root[F("sta_rssi")] = WiFi.RSSI();
    root[F("ap_status")] = ((WiFi.getMode() & WIFI_AP) != 0);
    root[F("ap_ssid")] = WiFiSettings.getApName();
    root[F("ap_ip")] = WiFi.softAPIP().toString();
    root[F("ap_mac")] = WiFi.softAPmacAddress();
    root[F("ap_stationnum")] = WiFi.softAPgetStationNum();

    response->setLength();
    request->send(response);
}

void WebApiClass::onSystemStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root[F("hostname")] = WiFi.getHostname();

    root[F("sdkversion")] = ESP.getSdkVersion();
    root[F("cpufreq")] = ESP.getCpuFreqMHz();

    root[F("heap_total")] = ESP.getHeapSize();
    root[F("heap_used")] = ESP.getHeapSize() - ESP.getFreeHeap();
    root[F("sketch_total")] = ESP.getFreeSketchSpace();
    root[F("sketch_used")] = ESP.getSketchSize();
    root[F("littlefs_total")] = LittleFS.totalBytes();
    root[F("littlefs_used")] = LittleFS.usedBytes();

    root[F("chiprevision")] = ESP.getChipRevision();
    root[F("chipmodel")] = ESP.getChipModel();
    root[F("chipcores")] = ESP.getChipCores();

    String reason;
    reason = ResetReason.get_reset_reason_verbose(0);
    root[F("resetreason_0")] = reason;

    reason = ResetReason.get_reset_reason_verbose(1);
    root[F("resetreason_1")] = reason;

    root[F("cfgsavecount")] = Configuration.get().Cfg_SaveCount;

    char version[16];
    sprintf(version, "%d.%d.%d", CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);
    root[F("firmware_version")] = version;

    root[F("uptime")] = esp_timer_get_time() / 1000000;

    response->setLength();
    request->send(response);
}

void WebApiClass::onNetworkAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("hostname")] = config.WiFi_Hostname;
    root[F("dhcp")] = config.WiFi_Dhcp;
    root[F("ipaddress")] = IPAddress(config.WiFi_Ip).toString();
    root[F("netmask")] = IPAddress(config.WiFi_Netmask).toString();
    root[F("gateway")] = IPAddress(config.WiFi_Gateway).toString();
    root[F("dns1")] = IPAddress(config.WiFi_Dns1).toString();
    root[F("dns2")] = IPAddress(config.WiFi_Dns2).toString();
    root[F("ssid")] = config.WiFi_Ssid;
    root[F("password")] = config.WiFi_Password;

    response->setLength();
    request->send(response);
}

void WebApiClass::onNetworkAdminPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("ssid") && root.containsKey("password") && root.containsKey("hostname") && root.containsKey("dhcp") && root.containsKey("ipaddress") && root.containsKey("netmask") && root.containsKey("gateway") && root.containsKey("dns1") && root.containsKey("dns2"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    IPAddress ipaddress;
    if (!ipaddress.fromString(root[F("ipaddress")].as<String>())) {
        retMsg[F("message")] = F("IP address is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress netmask;
    if (!netmask.fromString(root[F("netmask")].as<String>())) {
        retMsg[F("message")] = F("Netmask is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress gateway;
    if (!gateway.fromString(root[F("gateway")].as<String>())) {
        retMsg[F("message")] = F("Gateway is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress dns1;
    if (!dns1.fromString(root[F("dns1")].as<String>())) {
        retMsg[F("message")] = F("DNS Server IP 1 is invalid!");
        response->setLength();
        request->send(response);
        return;
    }
    IPAddress dns2;
    if (!dns2.fromString(root[F("dns2")].as<String>())) {
        retMsg[F("message")] = F("DNS Server IP 2 is invalid!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("hostname")].as<String>().length() == 0 || root[F("hostname")].as<String>().length() > WIFI_MAX_HOSTNAME_STRLEN) {
        retMsg[F("message")] = F("Hostname must between 1 and " STR(WIFI_MAX_HOSTNAME_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }
    if (root[F("ssid")].as<String>().length() == 0 || root[F("ssid")].as<String>().length() > WIFI_MAX_SSID_STRLEN) {
        retMsg[F("message")] = F("SSID must between 1 and " STR(WIFI_MAX_SSID_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }
    if (root[F("password")].as<String>().length() > WIFI_MAX_PASSWORD_STRLEN - 1) {
        retMsg[F("message")] = F("Password must not be longer than " STR(WIFI_MAX_PASSWORD_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    config.WiFi_Ip[0] = ipaddress[0];
    config.WiFi_Ip[1] = ipaddress[1];
    config.WiFi_Ip[2] = ipaddress[2];
    config.WiFi_Ip[3] = ipaddress[3];
    config.WiFi_Netmask[0] = netmask[0];
    config.WiFi_Netmask[1] = netmask[1];
    config.WiFi_Netmask[2] = netmask[2];
    config.WiFi_Netmask[3] = netmask[3];
    config.WiFi_Gateway[0] = gateway[0];
    config.WiFi_Gateway[1] = gateway[1];
    config.WiFi_Gateway[2] = gateway[2];
    config.WiFi_Gateway[3] = gateway[3];
    config.WiFi_Dns1[0] = dns1[0];
    config.WiFi_Dns1[1] = dns1[1];
    config.WiFi_Dns1[2] = dns1[2];
    config.WiFi_Dns1[3] = dns1[3];
    config.WiFi_Dns2[0] = dns2[0];
    config.WiFi_Dns2[1] = dns2[1];
    config.WiFi_Dns2[2] = dns2[2];
    config.WiFi_Dns2[3] = dns2[3];
    strcpy(config.WiFi_Ssid, root[F("ssid")].as<String>().c_str());
    strcpy(config.WiFi_Password, root[F("password")].as<String>().c_str());
    strcpy(config.WiFi_Hostname, root[F("hostname")].as<String>().c_str());
    if (root[F("dhcp")].as<bool>()) {
        config.WiFi_Dhcp = true;
    } else {
        config.WiFi_Dhcp = false;
    }
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    WiFiSettings.enableAdminMode();
    WiFiSettings.applyConfig();
}

void WebApiClass::onNtpStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("ntp_server")] = config.Ntp_Server;
    root[F("ntp_timezone")] = config.Ntp_Timezone;
    root[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        root[F("ntp_status")] = false;
    } else {
        root[F("ntp_status")] = true;
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    root[F("ntp_localtime")] = timeStringBuff;

    response->setLength();
    request->send(response);
}

void WebApiClass::onNtpAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("ntp_server")] = config.Ntp_Server;
    root[F("ntp_timezone")] = config.Ntp_Timezone;
    root[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    response->setLength();
    request->send(response);
}

void WebApiClass::onNtpAdminPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("ntp_server") && root.containsKey("ntp_timezone"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_server")].as<String>().length() == 0 || root[F("ntp_server")].as<String>().length() > NTP_MAX_SERVER_STRLEN) {
        retMsg[F("message")] = F("NTP Server must between 1 and " STR(NTP_MAX_SERVER_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone")].as<String>().length() == 0 || root[F("ntp_timezone")].as<String>().length() > NTP_MAX_TIMEZONE_STRLEN) {
        retMsg[F("message")] = F("Timezone must between 1 and " STR(NTP_MAX_TIMEZONE_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone_descr")].as<String>().length() == 0 || root[F("ntp_timezone_descr")].as<String>().length() > NTP_MAX_TIMEZONEDESCR_STRLEN) {
        retMsg[F("message")] = F("Timezone description must between 1 and " STR(NTP_MAX_TIMEZONEDESCR_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strcpy(config.Ntp_Server, root[F("ntp_server")].as<String>().c_str());
    strcpy(config.Ntp_Timezone, root[F("ntp_timezone")].as<String>().c_str());
    strcpy(config.Ntp_TimezoneDescr, root[F("ntp_timezone_descr")].as<String>().c_str());
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    NtpSettings.setServer();
    NtpSettings.setTimezone();
}

void WebApiClass::onMqttStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_connected")] = MqttSettings.getConnected();
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_lwt_topic")] = String(config.Mqtt_Topic) + config.Mqtt_LwtTopic;

    response->setLength();
    request->send(response);
}

void WebApiClass::onMqttAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    root[F("mqtt_enabled")] = config.Mqtt_Enabled;
    root[F("mqtt_hostname")] = config.Mqtt_Hostname;
    root[F("mqtt_port")] = config.Mqtt_Port;
    root[F("mqtt_username")] = config.Mqtt_Username;
    root[F("mqtt_password")] = config.Mqtt_Password;
    root[F("mqtt_topic")] = config.Mqtt_Topic;
    root[F("mqtt_retain")] = config.Mqtt_Retain;
    root[F("mqtt_lwt_topic")] = config.Mqtt_LwtTopic;
    root[F("mqtt_lwt_online")] = config.Mqtt_LwtValue_Online;
    root[F("mqtt_lwt_offline")] = config.Mqtt_LwtValue_Offline;

    response->setLength();
    request->send(response);
}

void WebApiClass::onMqttAdminPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("mqtt_enabled") && root.containsKey("mqtt_hostname") && root.containsKey("mqtt_port") && root.containsKey("mqtt_username") && root.containsKey("mqtt_password") && root.containsKey("mqtt_topic") && root.containsKey("mqtt_retain") && root.containsKey("mqtt_lwt_topic") && root.containsKey("mqtt_lwt_online") && root.containsKey("mqtt_lwt_offline"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("mqtt_enabled")].as<bool>()) {
        if (root[F("mqtt_hostname")].as<String>().length() == 0 || root[F("mqtt_hostname")].as<String>().length() > MQTT_MAX_HOSTNAME_STRLEN) {
            retMsg[F("message")] = F("MqTT Server must between 1 and " STR(MQTT_MAX_HOSTNAME_STRLEN) " characters long!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_username")].as<String>().length() > MQTT_MAX_USERNAME_STRLEN) {
            retMsg[F("message")] = F("Username must not longer then " STR(MQTT_MAX_USERNAME_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_password")].as<String>().length() > MQTT_MAX_PASSWORD_STRLEN) {
            retMsg[F("message")] = F("Password must not longer then " STR(MQTT_MAX_PASSWORD_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }
        if (root[F("mqtt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("Topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_port")].as<uint>() == 0 || root[F("mqtt_port")].as<uint>() > 65535) {
            retMsg[F("message")] = F("Port must be a number between 1 and 65535!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_topic")].as<String>().length() > MQTT_MAX_TOPIC_STRLEN) {
            retMsg[F("message")] = F("LWT topic must not longer then " STR(MQTT_MAX_TOPIC_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_online")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT online value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }

        if (root[F("mqtt_lwt_offline")].as<String>().length() > MQTT_MAX_LWTVALUE_STRLEN) {
            retMsg[F("message")] = F("LWT offline value must not longer then " STR(MQTT_MAX_LWTVALUE_STRLEN) " characters!");
            response->setLength();
            request->send(response);
            return;
        }
    }

    CONFIG_T& config = Configuration.get();
    config.Mqtt_Enabled = root[F("mqtt_enabled")].as<bool>();
    config.Mqtt_Retain = root[F("mqtt_retain")].as<bool>();
    config.Mqtt_Port = root[F("mqtt_port")].as<uint>();
    strcpy(config.Mqtt_Hostname, root[F("mqtt_hostname")].as<String>().c_str());
    strcpy(config.Mqtt_Username, root[F("mqtt_username")].as<String>().c_str());
    strcpy(config.Mqtt_Password, root[F("mqtt_password")].as<String>().c_str());
    strcpy(config.Mqtt_Topic, root[F("mqtt_topic")].as<String>().c_str());
    strcpy(config.Mqtt_LwtTopic, root[F("mqtt_lwt_topic")].as<String>().c_str());
    strcpy(config.Mqtt_LwtValue_Online, root[F("mqtt_lwt_online")].as<String>().c_str());
    strcpy(config.Mqtt_LwtValue_Offline, root[F("mqtt_lwt_offline")].as<String>().c_str());
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    MqttSettings.performReconnect();
}

void WebApiClass::onInverterList(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    JsonArray data = root.createNestedArray(F("inverter"));

    CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            JsonObject obj = data.createNestedObject();
            obj[F("id")] = i;
            obj[F("name")] = String(config.Inverter[i].Name);

            // Inverter Serial is read as HEX
            char buffer[sizeof(uint64_t) * 8 + 1];
            sprintf(buffer, "%0lx%08lx",
                ((uint32_t)((config.Inverter[i].Serial >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(config.Inverter[i].Serial & 0xFFFFFFFF)));
            obj[F("serial")] = buffer;
        }
    }

    response->setLength();
    request->send(response);
}

void WebApiClass::onInverterAdd(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("serial") && root.containsKey("name"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("name")].as<String>().length() == 0 || root[F("name")].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg[F("message")] = F("Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T* inverter = Configuration.getFreeInverterSlot();

    if (!inverter) {
        retMsg[F("message")] = F("Only " STR(INV_MAX_COUNT) " inverters are supported!");
        response->setLength();
        request->send(response);
        return;
    }

    char* t;
    // Interpret the string as a hex value and convert it to uint64_t
    inverter->Serial = strtoll(root[F("serial")].as<String>().c_str(), &t, 16);

    strncpy(inverter->Name, root[F("name")].as<String>().c_str(), INV_MAX_NAME_STRLEN);
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Inverter created!");

    response->setLength();
    request->send(response);

    Hoymiles.addInverter(inverter->Name, inverter->Serial);
}

void WebApiClass::onInverterEdit(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("id") && root.containsKey("serial") && root.containsKey("name"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("id")].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg[F("message")] = F("Invalid ID specified!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial must be a number > 0!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("name")].as<String>().length() == 0 || root[F("name")].as<String>().length() > INV_MAX_NAME_STRLEN) {
        retMsg[F("message")] = F("Name must between 1 and " STR(INV_MAX_NAME_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[root[F("id")].as<uint8_t>()];

    char* t;
    // Interpret the string as a hex value and convert it to uint64_t
    inverter.Serial = strtoll(root[F("serial")].as<String>().c_str(), &t, 16);
    strncpy(inverter.Name, root[F("name")].as<String>().c_str(), INV_MAX_NAME_STRLEN);
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Inverter changed!");

    response->setLength();
    request->send(response);

    std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByPos(root[F("id")].as<uint64_t>());
    inv->setName(inverter.Name);
    inv->setSerial(inverter.Serial);
}

void WebApiClass::onInverterDelete(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("id"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("id")].as<uint8_t>() > INV_MAX_COUNT - 1) {
        retMsg[F("message")] = F("Invalid ID specified!");
        response->setLength();
        request->send(response);
        return;
    }

    uint8_t inverter_id = root[F("id")].as<uint8_t>();
    INVERTER_CONFIG_T& inverter = Configuration.get().Inverter[inverter_id];
    inverter.Serial = 0;
    strncpy(inverter.Name, "", 0);
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Inverter deleted!");

    response->setLength();
    request->send(response);

    Hoymiles.removeInverterByPos(inverter_id);
}

void WebApiClass::onFirmwareUpdateFinish(AsyncWebServerRequest* request)
{
    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response

    AsyncWebServerResponse* response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

void WebApiClass::onFirmwareUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    // Upload handler chunks in data
    if (!index) {
        if (!request->hasParam("MD5", true)) {
            return request->send(400, "text/plain", "MD5 parameter missing");
        }

        if (!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
            return request->send(400, "text/plain", "MD5 parameter invalid");
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) { // Start with max available size
            Update.printError(Serial);
            return request->send(400, "text/plain", "OTA could not begin");
        }
    }

    // Write chunked data to the free sketch space
    if (len) {
        if (Update.write(data, len) != len) {
            return request->send(400, "text/plain", "OTA could not begin");
        }
    }

    if (final) { // if the final flag is set then this is the last frame of data
        if (!Update.end(true)) { // true to set the size to the current progress
            Update.printError(Serial);
            return request->send(400, "text/plain", "Could not end OTA");
        }
    } else {
        return;
    }
}

void WebApiClass::onDtuAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    CONFIG_T& config = Configuration.get();

    // DTU Serial is read as HEX
    char buffer[sizeof(uint64_t) * 8 + 1];
    sprintf(buffer, "%0lx%08lx",
        ((uint32_t)((config.Dtu_Serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(config.Dtu_Serial & 0xFFFFFFFF)));
    root[F("dtu_serial")] = buffer;
    root[F("dtu_pollinterval")] = config.Dtu_PollInterval;
    root[F("dtu_palevel")] = config.Dtu_PaLevel;

    response->setLength();
    request->send(response);
}

void WebApiClass::onDtuAdminPost(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("dtu_serial") && root.containsKey("dtu_pollinterval") && root.containsKey("dtu_palevel"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("dtu_serial")].as<uint64_t>() == 0) {
        retMsg[F("message")] = F("Serial cannot be zero!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("dtu_pollinterval")].as<uint32_t>() == 0) {
        retMsg[F("message")] = F("Poll interval must be greater zero!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("dtu_palevel")].as<uint8_t>() > 3) {
        retMsg[F("message")] = F("Invalid power level setting!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    char* t;
    // Interpret the string as a hex value and convert it to uint64_t
    config.Dtu_Serial = strtoll(root[F("dtu_serial")].as<String>().c_str(), &t, 16);
    config.Dtu_PollInterval = root[F("dtu_pollinterval")].as<uint32_t>();
    config.Dtu_PaLevel = root[F("dtu_palevel")].as<uint8_t>();
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    Hoymiles.getRadio()->setPALevel((rf24_pa_dbm_e)config.Dtu_PaLevel);
    Hoymiles.getRadio()->setDtuSerial(config.Dtu_Serial);
    Hoymiles.setPollInterval(config.Dtu_PollInterval);
}

WebApiClass WebApi;
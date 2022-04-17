#include "WebApi.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WiFiSettings.h"
#include "defaults.h"
#include "helper.h"
#include <LITTLEFS.h>
#include <ResetReason.h>

WebApiClass::WebApiClass()
    : _server(HTTP_PORT)
    , _ws("/ws")
    , _events("/events")
{
}

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

    _server.serveStatic("/", LITTLEFS, "/", "max-age=86400").setDefaultFile("index.html");
    _server.onNotFound(std::bind(&WebApiClass::onNotFound, this, _1));
    _server.begin();
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
    root[F("sketch_total")] = ESP.getSketchSize() + ESP.getFreeSketchSpace();
    root[F("sketch_used")] = ESP.getSketchSize();
    root[F("littlefs_total")] = LITTLEFS.totalBytes();
    root[F("littlefs_used")] = LITTLEFS.usedBytes();

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

    response->setLength();
    request->send(response);
}

WebApiClass WebApi;
#include "WebApi.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "WiFiSettings.h"
#include "defaults.h"
#include <LittleFS.h>

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

    _server.on("/api/network/status", HTTP_GET, std::bind(&WebApiClass::onNetworkStatus, this, _1));
    _server.on("/api/system/status", HTTP_GET, std::bind(&WebApiClass::onSystemStatus, this, _1));

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
    root[F("heapfree")] = ESP.getFreeHeap();
    root[F("heaptotal")] = ESP.getHeapSize();
    root[F("sdkversion")] = ESP.getSdkVersion();
    root[F("cpufreq")] = ESP.getCpuFreqMHz();
    root[F("sketchtotal")] = ESP.getSketchSize() + ESP.getFreeSketchSpace();
    root[F("sketchused")] = ESP.getSketchSize();

    response->setLength();
    request->send(response);
}

WebApiClass WebApi;
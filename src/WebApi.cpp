#include "WebApi.h"
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

WebApiClass WebApi;
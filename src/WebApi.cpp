#include "WebApi.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "defaults.h"

WebApiClass::WebApiClass()
    : _server(HTTP_PORT)
    , _ws("/livedata")
    , _events("/events")
{
}

void WebApiClass::init()
{
    using namespace std::placeholders;

    _server.addHandler(&_ws);
    _server.addHandler(&_events);

    _ws.onEvent(std::bind(&WebApiClass::onWebsocketEvent, this, _1, _2, _3, _4, _5, _6));

    _webApiDtu.init(&_server);
    _webApiFirmware.init(&_server);
    _webApiInverter.init(&_server);
    _webApiMqtt.init(&_server);
    _webApiNetwork.init(&_server);
    _webApiNtp.init(&_server);
    _webApiSysstatus.init(&_server);
    _webApiWebapp.init(&_server);

    _webApiWsLive.init(&_ws);

    _server.begin();
}

void WebApiClass::loop()
{
    _webApiDtu.loop();
    _webApiFirmware.loop();
    _webApiInverter.loop();
    _webApiMqtt.loop();
    _webApiNetwork.loop();
    _webApiNtp.loop();
    _webApiSysstatus.loop();
    _webApiWebapp.loop();

    _webApiWsLive.loop();

    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    if (millis() - lastTimerCall > 1000) {
        _ws.cleanupClients();
        lastTimerCall = millis();
    }
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
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiClass {
public:
    WebApiClass();
    void init();

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    AsyncEventSource _events;

    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    void onNotFound(AsyncWebServerRequest* request);

    void onNetworkStatus(AsyncWebServerRequest* request);
    void onSystemStatus(AsyncWebServerRequest* request);

    void onNetworkAdminGet(AsyncWebServerRequest* request);
    void onNetworkAdminPost(AsyncWebServerRequest* request);
};

extern WebApiClass WebApi;
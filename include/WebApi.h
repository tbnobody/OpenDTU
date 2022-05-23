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
    void onSystemStatus(AsyncWebServerRequest* request);

    void onNetworkStatus(AsyncWebServerRequest* request);
    void onNetworkAdminGet(AsyncWebServerRequest* request);
    void onNetworkAdminPost(AsyncWebServerRequest* request);

    void onNtpStatus(AsyncWebServerRequest* request);
    void onNtpAdminGet(AsyncWebServerRequest* request);
    void onNtpAdminPost(AsyncWebServerRequest* request);

    void onMqttStatus(AsyncWebServerRequest* request);
    void onMqttAdminGet(AsyncWebServerRequest* request);
    void onMqttAdminPost(AsyncWebServerRequest* request);

    void onInverterList(AsyncWebServerRequest* request);
    void onInverterAdd(AsyncWebServerRequest* request);
    void onInverterEdit(AsyncWebServerRequest* request);
    void onInverterDelete(AsyncWebServerRequest* request);

    void onFirmwareUpdateFinish(AsyncWebServerRequest* request);
    void onFirmwareUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

extern WebApiClass WebApi;
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiInverterClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onInverterList(AsyncWebServerRequest* request);
    void onInverterAdd(AsyncWebServerRequest* request);
    void onInverterEdit(AsyncWebServerRequest* request);
    void onInverterDelete(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
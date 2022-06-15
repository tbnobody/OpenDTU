#pragma once

#include <ESPAsyncWebServer.h>

class WebApiSysstatusClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onSystemStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
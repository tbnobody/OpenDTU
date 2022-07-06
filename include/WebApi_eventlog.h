#pragma once

#include <ESPAsyncWebServer.h>

class WebApiEventlogClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onEventlogStatus(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
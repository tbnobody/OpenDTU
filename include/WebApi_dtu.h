#pragma once

#include <ESPAsyncWebServer.h>

class WebApiDtuClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onDtuAdminGet(AsyncWebServerRequest* request);
    void onDtuAdminPost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiFirmwareClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onFirmwareUpdateFinish(AsyncWebServerRequest* request);
    void onFirmwareUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);

    AsyncWebServer* _server;
};
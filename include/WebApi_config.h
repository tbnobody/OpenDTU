// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiConfigClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onConfigGet(AsyncWebServerRequest* request);
    void onConfigDelete(AsyncWebServerRequest* request);
    void onConfigListGet(AsyncWebServerRequest* request);
    void onConfigUploadFinish(AsyncWebServerRequest* request);
    void onConfigUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);

    AsyncWebServer* _server;
};
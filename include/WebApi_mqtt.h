// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

#define MQTT_JSON_DOC_SIZE 3072

class WebApiMqttClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onMqttStatus(AsyncWebServerRequest* request);
    void onMqttAdminGet(AsyncWebServerRequest* request);
    void onMqttAdminPost(AsyncWebServerRequest* request);
    void setRootCaCertInfo(char* cert);
    char* getRootCaCertInfo();

    AsyncWebServer* _server;
    char _rootCaCertInfo[1024] ="";
};
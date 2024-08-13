// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

#define MQTT_JSON_DOC_SIZE 10240

class WebApiMqttClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onMqttStatus(AsyncWebServerRequest* request);
    void onMqttAdminGet(AsyncWebServerRequest* request);
    void onMqttAdminPost(AsyncWebServerRequest* request);
    String getTlsCertInfo(const char* cert);
};

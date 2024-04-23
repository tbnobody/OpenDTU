// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>
#include <ArduinoJson.h>
#include "Configuration.h"

class WebApiPowerMeterClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onStatus(AsyncWebServerRequest* request);
    void onAdminGet(AsyncWebServerRequest* request);
    void onAdminPost(AsyncWebServerRequest* request);
    void decodeJsonPhaseConfig(JsonObject const& json, PowerMeterHttpConfig& config) const;
    void decodeJsonTibberConfig(JsonObject const& json, PowerMeterTibberConfig& config) const;
    void onTestHttpRequest(AsyncWebServerRequest* request);
    void onTestTibberRequest(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

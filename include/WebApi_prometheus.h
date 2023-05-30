// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>

class WebApiPrometheusClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onPrometheusMetricsGet(AsyncWebServerRequest* request);

    void addField(AsyncResponseStream* stream, String& serial, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, const char* channelName = NULL);

    AsyncWebServer* _server;
};
#pragma once

#include "ArduinoJson.h"
#include <ESPAsyncWebServer.h>
#include <Hoymiles.h>

class WebApiWsLiveClass {
public:
    void init(AsyncWebSocket* ws);
    void loop();

private:
    AsyncWebSocket* _ws;
    void addField(JsonDocument& root, uint8_t idx, std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId, String topic = "");

    uint32_t _lastWsPublish = 0;
    uint32_t _lastInvUpdateCheck = 0;
    uint32_t _newestInverterTimestamp = 0;
};
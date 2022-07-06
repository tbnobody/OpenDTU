#pragma once

#include "WebApi_dtu.h"
#include "WebApi_firmware.h"
#include "WebApi_inverter.h"
#include "WebApi_mqtt.h"
#include "WebApi_network.h"
#include "WebApi_ntp.h"
#include "WebApi_sysstatus.h"
#include "WebApi_webapp.h"
#include "WebApi_ws_live.h"
#include <ESPAsyncWebServer.h>

class WebApiClass {
public:
    WebApiClass();
    void init();
    void loop();

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    AsyncEventSource _events;

    WebApiDtuClass _webApiDtu;
    WebApiFirmwareClass _webApiFirmware;
    WebApiInverterClass _webApiInverter;
    WebApiMqttClass _webApiMqtt;
    WebApiNetworkClass _webApiNetwork;
    WebApiNtpClass _webApiNtp;
    WebApiSysstatusClass _webApiSysstatus;
    WebApiWebappClass _webApiWebapp;
    WebApiWsLiveClass _webApiWsLive;
    unsigned long lastTimerCall = 0;

    void onWebsocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);
};

extern WebApiClass WebApi;
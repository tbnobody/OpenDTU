// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "WebApi_devinfo.h"
#include "WebApi_dtu.h"
#include "WebApi_eventlog.h"
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
    AsyncEventSource _events;

    WebApiDevInfoClass _webApiDevInfo;
    WebApiDtuClass _webApiDtu;
    WebApiEventlogClass _webApiEventlog;
    WebApiFirmwareClass _webApiFirmware;
    WebApiInverterClass _webApiInverter;
    WebApiMqttClass _webApiMqtt;
    WebApiNetworkClass _webApiNetwork;
    WebApiNtpClass _webApiNtp;
    WebApiSysstatusClass _webApiSysstatus;
    WebApiWebappClass _webApiWebapp;
    WebApiWsLiveClass _webApiWsLive;
};

extern WebApiClass WebApi;
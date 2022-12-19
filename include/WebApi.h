// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "WebApi_config.h"
#include "WebApi_devinfo.h"
#include "WebApi_dtu.h"
#include "WebApi_eventlog.h"
#include "WebApi_firmware.h"
#include "WebApi_inverter.h"
#include "WebApi_limit.h"
#include "WebApi_maintenance.h"
#include "WebApi_mqtt.h"
#include "WebApi_network.h"
#include "WebApi_ntp.h"
#include "WebApi_power.h"
#include "WebApi_prometheus.h"
#include "WebApi_security.h"
#include "WebApi_sysstatus.h"
#include "WebApi_webapp.h"
#include "WebApi_ws_console.h"
#include "WebApi_ws_live.h"
#include <ESPAsyncWebServer.h>

class WebApiClass {
public:
    WebApiClass();
    void init();
    void loop();

    static bool checkCredentials(AsyncWebServerRequest* request);
    static bool checkCredentialsReadonly(AsyncWebServerRequest* request);

private:
    AsyncWebServer _server;
    AsyncEventSource _events;

    WebApiConfigClass _webApiConfig;
    WebApiDevInfoClass _webApiDevInfo;
    WebApiDtuClass _webApiDtu;
    WebApiEventlogClass _webApiEventlog;
    WebApiFirmwareClass _webApiFirmware;
    WebApiInverterClass _webApiInverter;
    WebApiLimitClass _webApiLimit;
    WebApiMaintenanceClass _webApiMaintenance;
    WebApiMqttClass _webApiMqtt;
    WebApiNetworkClass _webApiNetwork;
    WebApiNtpClass _webApiNtp;
    WebApiPowerClass _webApiPower;
    WebApiPrometheusClass _webApiPrometheus;
    WebApiSecurityClass _webApiSecurity;
    WebApiSysstatusClass _webApiSysstatus;
    WebApiWebappClass _webApiWebapp;
    WebApiWsConsoleClass _webApiWsConsole;
    WebApiWsLiveClass _webApiWsLive;
};

extern WebApiClass WebApi;
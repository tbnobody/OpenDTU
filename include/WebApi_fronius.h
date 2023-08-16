// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiFroniusClass {
private:
    // See Victron dbus_fronius / fronius_device_info.cpp
    static const auto FRONIUS_INVERTER_TYPE_1PHASE = 81;
    static const auto FRONIUS_INVERTER_TYPE_3PHASE = 125;

public:
    void init(AsyncWebServer* server);
    void loop();

private:
    void onGetActiveDeviceInfo(AsyncWebServerRequest* request);
    void onGetInverterInfo(AsyncWebServerRequest* request);
    void NotFound(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiInverterClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onInverterList(AsyncWebServerRequest* request);
    void onInverterAdd(AsyncWebServerRequest* request);
    void onInverterEdit(AsyncWebServerRequest* request);
    void onInverterDelete(AsyncWebServerRequest* request);
    void onInverterOrder(AsyncWebServerRequest* request);
    void onInverterStatReset(AsyncWebServerRequest* request);
};

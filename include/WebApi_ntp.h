// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiNtpClass {
public:
    void init(AsyncWebServer& server);
    void loop();

private:
    void onNtpStatus(AsyncWebServerRequest* request);
    void onNtpAdminGet(AsyncWebServerRequest* request);
    void onNtpAdminPost(AsyncWebServerRequest* request);
    void onNtpTimeGet(AsyncWebServerRequest* request);
    void onNtpTimePost(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
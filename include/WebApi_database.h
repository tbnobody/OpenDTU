// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#define DATABASE_FILENAME "/database.bin"

class WebApiDatabaseClass {
public:
    void init(AsyncWebServer* server);
    void loop();
    bool write(float energy);

    struct Data {
        uint8_t	tm_year;
        uint8_t tm_mon;
        uint8_t tm_mday;
        uint8_t tm_hour;
        float   energy;
    };

private:
    void onDatabase(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};

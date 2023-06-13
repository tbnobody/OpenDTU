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

    struct pvData {
        uint8_t	tm_year;
        uint8_t tm_mon;
        uint8_t tm_mday;
        uint8_t tm_hour;
        float   energy;
    };

private:
    void onDatabase(AsyncWebServerRequest* request);
    static size_t readchunk(uint8_t* buffer, size_t maxLen, size_t index);
    static size_t readchunk1(uint8_t* buffer, size_t maxLen, size_t index);
    
    AsyncWebServer* _server;
};

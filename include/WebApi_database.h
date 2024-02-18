// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

#define DATABASE_FILENAME "/database.bin"

class WebApiDatabaseClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);
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
    void onDatabaseHour(AsyncWebServerRequest* request);
    void onDatabaseDay(AsyncWebServerRequest* request);
    static size_t readchunk(uint8_t* buffer, size_t maxLen, size_t index);
    static size_t readchunk_log(uint8_t* buffer, size_t maxLen, size_t index);
    static size_t readchunkHour(uint8_t* buffer, size_t maxLen, size_t index);
    static size_t readchunkDay(uint8_t* buffer, size_t maxLen, size_t index);
    
    AsyncWebServer* _server;
};

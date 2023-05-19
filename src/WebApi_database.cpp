// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Ralf Bauer and others
 */

#include "WebApi_database.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiDatabaseClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;
    _server->on("/api/database", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabase, this, _1));
}

void WebApiDatabaseClass::loop()
{
}

bool WebApiDatabaseClass::write(float energy)
{
    static uint8_t old_hour = 255;
    static float old_energy = 0.0;

    //LittleFS.remove(DATABASE_FILENAME);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5))
        return(false);
    if (timeinfo.tm_hour == old_hour)
        return(false);
    if (energy <= old_energy)
        return(false);

    struct Data d;
    d.tm_hour = old_hour = timeinfo.tm_hour;
    d.tm_year = timeinfo.tm_year - 100; // year counting from 2000
    d.tm_mon = timeinfo.tm_mon + 1;
    d.tm_mday = timeinfo.tm_mday;
    d.energy = old_energy = energy;

    File f = LittleFS.open(DATABASE_FILENAME, "a");
    if (!f) {
        return(false);
    }
    f.write((const uint8_t*)&d, sizeof(Data));
    f.close();
    return(true);
}

void WebApiDatabaseClass::onDatabase(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    try {
        File f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return;
        }

        struct Data d;

        AsyncJsonResponse* response = new AsyncJsonResponse(true, 40000U);
        JsonArray root = response->getRoot();

        while (f.read((uint8_t*)&d, sizeof(Data))) {
            JsonArray nested = root.createNestedArray();
            nested.add(d.tm_year);
            nested.add(d.tm_mon);
            nested.add(d.tm_mday);
            nested.add(d.tm_hour);
            nested.add(d.energy);
        }
        f.close();

        response->setLength();
        request->send(response);

    } catch (std::bad_alloc& bad_alloc) {
        MessageOutput.printf("Call to /api/database temporarely out of resources. Reason: \"%s\".\r\n", bad_alloc.what());

        WebApi.sendTooManyRequests(request);
    }
}

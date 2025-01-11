// SPDX-License-Identifier: GPL-2.0-or-later

#include "WebApi_database.h"
#include "WebApi.h"
#include "Datastore.h"
#include "MessageOutput.h"
#include "defaults.h"
#include <Arduino.h>
#include <AsyncJson.h>
#include <LittleFS.h>


WebApiDatabaseClass::WebApiDatabaseClass()
    : _sendDataTask(1 * TASK_MINUTE, TASK_FOREVER, std::bind(&WebApiDatabaseClass::sendDataTaskCb, this))
{
}

void WebApiDatabaseClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/database", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabase, this, _1));
    server.on("/api/databaseHour", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabaseHour, this, _1));
    server.on("/api/databaseDay", HTTP_GET, std::bind(&WebApiDatabaseClass::onDatabaseDay, this, _1));

    scheduler.addTask(_sendDataTask);
    _sendDataTask.enable();
}

void WebApiDatabaseClass::sendDataTaskCb()
{
    if (!Hoymiles.isAllRadioIdle()) {
        return;
    }
    write(Datastore.getTotalAcYieldTotalEnabled()); // write value to database
}

bool WebApiDatabaseClass::write(float energy)
{
    static uint8_t old_hour = 255;
    static float old_energy = 0.0;

    // LittleFS.remove(DATABASE_FILENAME);

    // MessageOutput.println(energy, 6);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return false;
    }
    if (timeinfo.tm_hour == old_hour) // must be new hour
        return (false);
    if (old_hour == 255) { // don't write to database after reboot
        old_hour = timeinfo.tm_hour;
        return (false);
    }
    // MessageOutput.println("Next hour.");
    if (energy <= old_energy) // enery must have increased
        return (false);
    // MessageOutput.println("Energy difference > 0");

    struct pvData d;
    d.tm_hour = timeinfo.tm_hour - 1;
    old_hour = timeinfo.tm_hour;
    d.tm_year = timeinfo.tm_year - 100; // year counting from 2000
    d.tm_mon = timeinfo.tm_mon + 1;
    d.tm_mday = timeinfo.tm_mday;
    d.energy = old_energy = energy;

    // create database file if it does not exist
    // if (!LittleFS.exists(DATABASE_FILENAME)) {
    //    MessageOutput.println("Database file does not exist.");
    //    File f = LittleFS.open(DATABASE_FILENAME, "w", true);
    //    f.flush();
    //    f.close();
    //    MessageOutput.println("New database file created.");
    //}

    File f = LittleFS.open(DATABASE_FILENAME, "a", true);
    if (!f) {
        MessageOutput.println("Failed to append the database.");
        return (false);
    }
    f.write((const uint8_t*)&d, sizeof(pvData));
    f.close();
    // MessageOutput.println("Write data point.");
    return (true);
}

// read chunk from database
size_t WebApiDatabaseClass::readchunk(uint8_t* buffer, size_t maxLen, size_t index)
{
    static bool first = true;
    static bool last = false;
    static File f;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (first) {
        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (0);
        }
        *pr++ = '[';
    }
    while (true) {
        r = f.read(reinterpret_cast<uint8_t*>(&d), sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                first = true;
                last = false;
                return (0); // end transmission
            }
            last = true;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (first) {
            first = false;
        } else {
            *pr++ = ',';
        }
        int len = snprintf(reinterpret_cast<char*>(pr), maxLen, "[%d,%d,%d,%d,%f]",
            d.tm_year, d.tm_mon, d.tm_mday, d.tm_hour, d.energy * 1e3);
        if (len >= 0) {
            pr += len;
        }
        if (pr >= pre)
            return (pr - buffer); // buffer full, return number of chars
    }
}

size_t WebApiDatabaseClass::readchunk_log(uint8_t* buffer, size_t maxLen, size_t index)
{
    size_t x = readchunk(buffer, maxLen, index);
    //MessageOutput.println("----------");
    //MessageOutput.println(maxLen);
    //MessageOutput.println(x);
    return (x);
}

// read chunk from database for the last 25 hours
size_t WebApiDatabaseClass::readchunkHour(uint8_t* buffer, size_t maxLen, size_t index)
{
    static bool first = true;
    static bool last = false;
    static bool valid = false;
    static float oldenergy = 0.0;
    static File f;
    static bool fileopen = false;
    union datehour {
        uint32_t dh;
        uint8_t dd[4];
    };
    static datehour startdate;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (!fileopen) {
        time_t now;
        struct tm sdate;
        time(&now);
        time_t stime = now - (60 * 60 * 25); // subtract 25h
        localtime_r(&stime, &sdate);
        if (sdate.tm_year <= (2016 - 1900)) {
            return (false); // time not set
        }
        startdate.dd[3] = sdate.tm_year - 100;
        startdate.dd[2] = sdate.tm_mon + 1;
        startdate.dd[1] = sdate.tm_mday;
        startdate.dd[0] = sdate.tm_hour;

        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (false);
        }
        fileopen = true;
        *pr++ = '[';
    }
    while (true) {
        r = f.read(reinterpret_cast<uint8_t*>(&d), sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                fileopen = false;
                first = true;
                last = false;
                valid = false;
                startdate.dh = 0L;
                return (0); // end transmission
            }
            last = true;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (!valid) {
            datehour cd;
            cd.dd[3] = d.tm_year;
            cd.dd[2] = d.tm_mon;
            cd.dd[1] = d.tm_mday;
            cd.dd[0] = d.tm_hour;
            // MessageOutput.println(cd,16);
            if ((cd.dh >= startdate.dh) && (oldenergy > 0.0)) {
                valid = true;
            } else
                oldenergy = d.energy;
        }
        if (valid) {
            if (first) {
                first = false;
            } else {
                *pr++ = ',';
            }
            int len = snprintf(reinterpret_cast<char*>(pr), maxLen, "[%d,%d,%d,%d,%f]",
                d.tm_year, d.tm_mon, d.tm_mday, d.tm_hour,
                (d.energy - oldenergy) * 1e3);
            oldenergy = d.energy;
            if (len >= 0) {
                pr += len;
            }
            if (pr >= pre) {
                return (pr - buffer); // buffer full, return number of chars
            }
        }
    }
}

// read chunk from database for calendar view
size_t WebApiDatabaseClass::readchunkDay(uint8_t* buffer, size_t maxLen, size_t index)
{
    static bool first = true;
    static bool last = false;
    static float startenergy = 0.0;
    static struct pvData endofday = { 0, 0, 0, 0, 0.0 };
    static File f;
    uint8_t* pr = buffer;
    uint8_t* pre = pr + maxLen - 50;
    size_t r;
    struct pvData d;

    if (first) {
        f = LittleFS.open(DATABASE_FILENAME, "r", false);
        if (!f) {
            return (0);
        }
        *pr++ = '[';
    }
    while (true) {
        r = f.read(reinterpret_cast<uint8_t*>(&d), sizeof(pvData)); // read from database
        if (r <= 0) {
            if (last) {
                f.close();
                first = true;
                last = false;
                endofday = { 0, 0, 0, 0, 0.0 };
                startenergy = 0.0;
                return (0); // end transmission
            }
            last = true;
            if (!first)
                *pr++ = ',';
            int len = snprintf(reinterpret_cast<char*>(pr), maxLen, "[%d,%d,%d,%d,%f]",
                endofday.tm_year, endofday.tm_mon, endofday.tm_mday, endofday.tm_hour,
                (endofday.energy - startenergy) * 1e3);
            pr += len;
            *pr++ = ']';
            return (pr - buffer); // last chunk
        }
        if (startenergy == 0.0) {
            if (d.energy > 0.0) {
                startenergy = d.energy;
            }
        } else {
            if (endofday.tm_mday != d.tm_mday) { // next day
                if (first) {
                    first = false;
                } else
                    *pr++ = ',';
                int len = snprintf(reinterpret_cast<char*>(pr), maxLen, "[%d,%d,%d,%d,%f]",
                    endofday.tm_year, endofday.tm_mon, endofday.tm_mday, endofday.tm_hour,
                    (endofday.energy - startenergy) * 1e3);
                startenergy = endofday.energy;
                if (len >= 0)
                    pr += len;
                if (pr >= pre)
                    return (pr - buffer); // buffer full, return number of chars
            }
        }
        endofday = d;
    }
}

void WebApiDatabaseClass::onDatabase(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunk);
    request->send(response);
}

void WebApiDatabaseClass::onDatabaseHour(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunkHour);
    request->send(response);
}

void WebApiDatabaseClass::onDatabaseDay(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }
    AsyncWebServerResponse* response = request->beginChunkedResponse("application/json", readchunkDay);
    request->send(response);
}

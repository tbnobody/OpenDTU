// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_eventlog.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiEventlogClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/eventlog/status", HTTP_GET, std::bind(&WebApiEventlogClass::onEventlogStatus, this, _1));
}

void WebApiEventlogClass::loop()
{
}

void WebApiEventlogClass::onEventlogStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 2048);
    JsonObject root = response->getRoot();

    uint64_t serial = 0;
    if (request->hasParam("inv")) {
        String s = request->getParam("inv")->value();
        serial = strtoll(s.c_str(), NULL, 16);
    }

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        String serial = inv->serialString();

        uint8_t logEntryCount = inv->EventLog()->getEntryCount();

        root[serial]["count"] = logEntryCount;
        JsonArray eventsArray = root[serial].createNestedArray(F("events"));

        for (uint8_t logEntry = 0; logEntry < logEntryCount; logEntry++) {
            JsonObject eventsObject = eventsArray.createNestedObject();

            AlarmLogEntry_t entry;
            inv->EventLog()->getLogEntry(logEntry, &entry);

            eventsObject[F("message_id")] = entry.MessageId;
            eventsObject[F("message")] = entry.Message;
            eventsObject[F("start_time")] = entry.StartTime;
            eventsObject[F("end_time")] = entry.EndTime;
        }
    }

    response->setLength();
    request->send(response);
}
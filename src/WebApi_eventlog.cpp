// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_eventlog.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiEventlogClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/eventlog/status", HTTP_GET, std::bind(&WebApiEventlogClass::onEventlogStatus, this, _1));
}

void WebApiEventlogClass::onEventlogStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse(false, 2048);
    auto& root = response->getRoot();

    uint64_t serial = 0;
    if (request->hasParam("inv")) {
        String s = request->getParam("inv")->value();
        serial = strtoll(s.c_str(), NULL, 16);
    }

    AlarmMessageLocale_t locale = AlarmMessageLocale_t::EN;
    if (request->hasParam("locale")) {
        String s = request->getParam("locale")->value();
        s.toLowerCase();
        if (s == "de") {
            locale = AlarmMessageLocale_t::DE;
        }
        if (s == "fr") {
            locale = AlarmMessageLocale_t::FR;
        }
    }

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv != nullptr) {
        uint8_t logEntryCount = inv->EventLog()->getEntryCount();

        root["count"] = logEntryCount;
        JsonArray eventsArray = root.createNestedArray("events");

        for (uint8_t logEntry = 0; logEntry < logEntryCount; logEntry++) {
            JsonObject eventsObject = eventsArray.createNestedObject();

            AlarmLogEntry_t entry;
            inv->EventLog()->getLogEntry(logEntry, entry, locale);

            eventsObject["message_id"] = entry.MessageId;
            eventsObject["message"] = entry.Message;
            eventsObject["start_time"] = entry.StartTime;
            eventsObject["end_time"] = entry.EndTime;
        }
    }

    response->setLength();
    request->send(response);
}

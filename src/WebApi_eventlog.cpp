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

    server.on("/api/eventlog/status", HTTP_GET, std::bind(&WebApiEventlogClass::onEventlogStatus, this, _1));
}

void WebApiEventlogClass::onEventlogStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto serial = WebApi.parseSerialFromRequest(request);

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
        JsonArray eventsArray = root["events"].to<JsonArray>();

        for (uint8_t logEntry = 0; logEntry < logEntryCount; logEntry++) {
            JsonObject eventsObject = eventsArray.add<JsonObject>();

            AlarmLogEntry_t entry;
            inv->EventLog()->getLogEntry(logEntry, entry, locale);

            eventsObject["message_id"] = entry.MessageId;
            eventsObject["message"] = entry.Message;
            eventsObject["start_time"] = entry.StartTime;
            eventsObject["end_time"] = entry.EndTime;
        }
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

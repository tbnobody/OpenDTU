#include "WebApi_eventlog.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Hoymiles.h"

void WebApiEventlogClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;

    _server->on("/api/eventlog/status", HTTP_GET, std::bind(&WebApiEventlogClass::onEventlogStatus, this, _1));
}

void WebApiEventlogClass::loop()
{
}

void WebApiEventlogClass::onEventlogStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        // Inverter Serial is read as HEX
        char buffer[sizeof(uint64_t) * 8 + 1];
        sprintf(buffer, "%0lx%08lx",
            ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
            ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

        uint8_t logEntryCount = inv->EventLog()->getEntryCount();

        root[buffer]["count"] = logEntryCount;
        JsonArray eventsArray = root[buffer].createNestedArray(F("events"));

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
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WebApi_history.h"
#include "HistoricalDatastore.h"
#include "WebApi.h"
#include <AsyncJson.h>

WebApiHistoryClass WebApiHistory;

void WebApiHistoryClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/history/data", HTTP_GET, std::bind(&WebApiHistoryClass::onHistoryData, this, _1));
    server.on("/api/history/reset", HTTP_POST, std::bind(&WebApiHistoryClass::onHistoryReset, this, _1));
}

void WebApiHistoryClass::onHistoryData(AsyncWebServerRequest* request)
{
    // For ArduinoJson v7, the constructor only takes a single parameter (isArray)
    AsyncJsonResponse* response = new AsyncJsonResponse(false);
    JsonVariant root = response->getRoot();

    // Parse query parameters
    uint8_t resolution = 1; // Default to hourly
    time_t startTime = 0;
    time_t endTime = time(nullptr);

    if (request->hasParam("resolution")) {
        resolution = request->getParam("resolution")->value().toInt();
    }

    if (request->hasParam("start")) {
        startTime = request->getParam("start")->value().toInt();
    }

    if (request->hasParam("end")) {
        endTime = request->getParam("end")->value().toInt();
    }

    // Get data points for the requested time range and resolution
    DataPointCollection dataPoints = HistoricalDatastore.getDataRange(startTime, endTime, resolution);

    // Add information about the request
    JsonObject info = root["info"].to<JsonObject>();
    info["resolution"] = resolution;
    info["start_time"] = startTime;
    info["end_time"] = endTime;
    info["point_count"] = dataPoints.size();

    // Create arrays for timestamps, yield values, and peak power values
    JsonArray timestamps = root["timestamps"].to<JsonArray>();
    JsonArray yields = root["yields"].to<JsonArray>();
    JsonArray peakPowers = root["peak_powers"].to<JsonArray>();

    // Populate arrays with data points
    for (const auto& point : dataPoints) {
        timestamps.add(point.timestamp);
        yields.add(point.yieldValue);
        peakPowers.add(point.peakPowerValue);
    }

    // Calculate the content length after populating the JSON document
    response->setLength();
    request->send(response);
}

void WebApiHistoryClass::onHistoryReset(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();

    HistoricalDatastore.resetData();

    retMsg["type"] = "success";
    retMsg["message"] = "Historical data has been reset";

    response->setLength();
    request->send(response);
}

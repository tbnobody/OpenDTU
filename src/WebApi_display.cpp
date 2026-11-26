// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "WebApi_display.h"
#include "Display_Graphic.h"
#include "WebApi.h"
#include <AsyncJson.h>

void WebApiDisplayClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/display/history", HTTP_GET, static_cast<ArRequestHandlerFunction>(std::bind(&WebApiDisplayClass::onDisplayHistory, this, _1)));
}

void WebApiDisplayClass::onDisplayHistory(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    JsonArray values = root["values"].to<JsonArray>();
    const auto& graphValues = Display.Diagram().getGraphValues();
    const uint8_t graphValuesCount = Display.Diagram().getGraphValuesCount();
    for (uint8_t i = 0; i < graphValuesCount; i++) {
        values.add(graphValues[i]);
    }

    root["values_count"] = graphValuesCount;
    root["seconds_per_dot"] = Display.Diagram().getSecondsPerDot();

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

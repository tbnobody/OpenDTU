// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiWebappClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void responseBinaryDataWithETagCache(AsyncWebServerRequest* request, const String& contentType, const String& contentEncoding, const uint8_t* content, size_t len);
};

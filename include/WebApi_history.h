// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiHistoryClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onHistoryData(AsyncWebServerRequest* request);
    void onHistoryReset(AsyncWebServerRequest* request);
};

extern WebApiHistoryClass WebApiHistory;

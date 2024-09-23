// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiFirmwareClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onFirmwareUpdateFinish(AsyncWebServerRequest* request);
    void onFirmwareUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiFileClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onFileGet(AsyncWebServerRequest* request);
    void onFileDelete(AsyncWebServerRequest* request);
    void onFileDeleteAll(AsyncWebServerRequest* request);
    void onFileListGet(AsyncWebServerRequest* request);
    void onFileUploadFinish(AsyncWebServerRequest* request);
    void onFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
};

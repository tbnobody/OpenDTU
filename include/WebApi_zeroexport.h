// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiZeroExportClass {
public:
    // Register the read-only status and authenticated configuration endpoints.
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    // Return live grid power, regulation state, and effective settings.
    void onZeroExportStatus(AsyncWebServerRequest* request);
    // Return the editable persistent configuration.
    void onZeroExportAdminGet(AsyncWebServerRequest* request);
    // Validate, persist, and apply the submitted configuration.
    void onZeroExportAdminPost(AsyncWebServerRequest* request);
};

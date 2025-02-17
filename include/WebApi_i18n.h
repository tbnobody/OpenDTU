// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiI18nClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onI18nLanguages(AsyncWebServerRequest* request);
    void onI18nLanguage(AsyncWebServerRequest* request);
};

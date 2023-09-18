// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <ESPAsyncWebServer.h>

class WebApiSafeLimitClass {
public:
    void init(AsyncWebServer* server);
    void loop();

private:
    struct Fallback {
      Fallback(): _inverterSerial(0) {}
      uint64_t _inverterSerial;
      int32_t _timeoutMillis;
      int16_t _currentLimit;
    };

    void onSafeLimitPost(AsyncWebServerRequest* request);
    Fallback *getFallback(uint64_t inverterSerial);

    AsyncWebServer* _server;
    Fallback _fallback[INV_MAX_COUNT];
};


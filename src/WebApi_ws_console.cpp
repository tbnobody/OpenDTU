// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "WebApi_ws_console.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"

WebApiWsConsoleClass::WebApiWsConsoleClass()
    : _ws("/console")
{
}

void WebApiWsConsoleClass::init(AsyncWebServer& server)
{
    _server = &server;
    _server->addHandler(&_ws);
    MessageOutput.register_ws_output(&_ws);
}

void WebApiWsConsoleClass::loop()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    if (millis() - _lastWsCleanup > 1000) {
        _ws.cleanupClients();

        if (Configuration.get().Security.AllowReadonly) {
            _ws.setAuthentication("", "");
        } else {
            _ws.setAuthentication(AUTH_USERNAME, Configuration.get().Security.Password);
        }

        _lastWsCleanup = millis();
    }
}
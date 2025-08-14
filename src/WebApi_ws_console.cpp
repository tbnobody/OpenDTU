// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "WebApi_ws_console.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "defaults.h"

WebApiWsConsoleClass::WebApiWsConsoleClass()
    : _ws("/console")
    , _wsCleanupTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&WebApiWsConsoleClass::wsCleanupTaskCb, this))
{
}

void WebApiWsConsoleClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    server.addHandler(&_ws);
    MessageOutput.register_ws_output(&_ws);

    scheduler.addTask(_wsCleanupTask);
    _wsCleanupTask.enable();

    _simpleDigestAuth.setUsername(AUTH_USERNAME);
    _simpleDigestAuth.setRealm("console websocket");

    reload();
}

void WebApiWsConsoleClass::reload()
{
    _ws.removeMiddleware(&_simpleDigestAuth);

    auto const& config = Configuration.get();

    if (config.Security.AllowReadonly) {
        return;
    }

    _ws.enable(false);
    _simpleDigestAuth.setPassword(config.Security.Password);
    _ws.addMiddleware(&_simpleDigestAuth);
    _ws.closeAll();
    _ws.enable(true);
}

void WebApiWsConsoleClass::wsCleanupTaskCb()
{
    // see: https://github.com/me-no-dev/ESPAsyncWebServer#limiting-the-number-of-web-socket-clients
    _ws.cleanupClients();
}

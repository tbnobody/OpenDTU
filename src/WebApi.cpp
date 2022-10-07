// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "defaults.h"

WebApiClass::WebApiClass()
    : _server(HTTP_PORT)
    , _events("/events")
{
}

void WebApiClass::init()
{
    _server.addHandler(&_events);

    _webApiConfig.init(&_server);
    _webApiDevInfo.init(&_server);
    _webApiDtu.init(&_server);
    _webApiEventlog.init(&_server);
    _webApiFirmware.init(&_server);
    _webApiInverter.init(&_server);
    _webApiLimit.init(&_server);
    _webApiMqtt.init(&_server);
    _webApiNetwork.init(&_server);
    _webApiNtp.init(&_server);
    _webApiPower.init(&_server);
    _webApiSysstatus.init(&_server);
    _webApiWebapp.init(&_server);
    _webApiWsLive.init(&_server);
    _webApiWsVedirectLive.init(&_server);
    _webApiVedirect.init(&_server);

    _server.begin();
}

void WebApiClass::loop()
{
    _webApiConfig.loop();
    _webApiDevInfo.loop();
    _webApiDtu.loop();
    _webApiEventlog.loop();
    _webApiFirmware.loop();
    _webApiInverter.loop();
    _webApiLimit.loop();
    _webApiMqtt.loop();
    _webApiNetwork.loop();
    _webApiNtp.loop();
    _webApiPower.loop();
    _webApiSysstatus.loop();
    _webApiWebapp.loop();
    _webApiWsLive.loop();
    _webApiWsVedirectLive.loop();
    _webApiVedirect.loop();
}

WebApiClass WebApi;
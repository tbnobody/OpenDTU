// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "defaults.h"
#include <AsyncJson.h>

WebApiClass::WebApiClass()
    : _server(HTTP_PORT)
{
}

void WebApiClass::init(Scheduler& scheduler)
{
    _webApiDevice.init(_server, scheduler);
    _webApiDevInfo.init(_server, scheduler);
    _webApiDtu.init(_server, scheduler);
    _webApiEventlog.init(_server, scheduler);
    _webApiFile.init(_server, scheduler);
    _webApiFirmware.init(_server, scheduler);
    _webApiGridprofile.init(_server, scheduler);
    _webApiI18n.init(_server, scheduler);
    _webApiInverter.init(_server, scheduler);
    _webApiLimit.init(_server, scheduler);
    _webApiMaintenance.init(_server, scheduler);
    _webApiMqtt.init(_server, scheduler);
    _webApiNetwork.init(_server, scheduler);
    _webApiNtp.init(_server, scheduler);
    _webApiPower.init(_server, scheduler);
    _webApiPrometheus.init(_server, scheduler);
    _webApiSecurity.init(_server, scheduler);
    _webApiSysstatus.init(_server, scheduler);
    _webApiWebapp.init(_server, scheduler);
    _webApiWsConsole.init(_server, scheduler);
    _webApiWsLive.init(_server, scheduler);

    _server.begin();
}

void WebApiClass::reload()
{
    _webApiWsConsole.reload();
    _webApiWsLive.reload();
}

bool WebApiClass::checkCredentials(AsyncWebServerRequest* request)
{
    auto const& config = Configuration.get();
    if (request->authenticate(AUTH_USERNAME, config.Security.Password)) {
        return true;
    }

    AsyncWebServerResponse* r = request->beginResponse(401);

    // WebAPI should set the X-Requested-With to prevent browser internal auth dialogs
    if (!request->hasHeader("X-Requested-With")) {
        r->addHeader("WWW-Authenticate", "Basic realm=\"Login Required\"");
    }
    request->send(r);

    return false;
}

bool WebApiClass::checkCredentialsReadonly(AsyncWebServerRequest* request)
{
    auto const& config = Configuration.get();
    if (config.Security.AllowReadonly) {
        return true;
    } else {
        return checkCredentials(request);
    }
}

void WebApiClass::sendTooManyRequests(AsyncWebServerRequest* request)
{
    auto response = request->beginResponse(429, "text/plain", "Too Many Requests");
    response->addHeader("Retry-After", "60");
    request->send(response);
}

void WebApiClass::writeConfig(JsonVariant& retMsg, const WebApiError code, const String& message)
{
    if (!Configuration.write()) {
        retMsg["message"] = "Write failed!";
        retMsg["code"] = WebApiError::GenericWriteFailed;
    } else {
        retMsg["type"] = "success";
        retMsg["message"] = message;
        retMsg["code"] = code;
    }
}

bool WebApiClass::parseRequestData(AsyncWebServerRequest* request, AsyncJsonResponse* response, JsonDocument& json_document)
{
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return false;
    }

    const String json = request->getParam("data", true)->value();
    const DeserializationError error = deserializeJson(json_document, json);
    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}

uint64_t WebApiClass::parseSerialFromRequest(AsyncWebServerRequest* request, String param_name)
{
    if (request->hasParam(param_name)) {
        String s = request->getParam(param_name)->value();
        return strtoll(s.c_str(), NULL, 16);
    }

    return 0;
}

bool WebApiClass::sendJsonResponse(AsyncWebServerRequest* request, AsyncJsonResponse* response, const char* function, const uint16_t line)
{
    bool ret_val = true;
    if (response->overflowed()) {
        auto& root = response->getRoot();

        root.clear();
        root["message"] = String("500 Internal Server Error: ") + function + ", " + line;
        root["code"] = WebApiError::GenericInternalServerError;
        root["type"] = "danger";
        response->setCode(500);
        MessageOutput.printf("WebResponse failed: %s, %" PRIu16 "\r\n", function, line);
        ret_val = false;
    }

    response->setLength();
    request->send(response);
    return ret_val;
}

WebApiClass WebApi;

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_security.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WebApi.h"
#include "helper.h"

void WebApiSecurityClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/security/config", HTTP_GET, std::bind(&WebApiSecurityClass::onSecurityGet, this, _1));
    _server->on("/api/security/config", HTTP_POST, std::bind(&WebApiSecurityClass::onSecurityPost, this, _1));
    _server->on("/api/security/authenticate", HTTP_GET, std::bind(&WebApiSecurityClass::onAuthenticateGet, this, _1));
}

void WebApiSecurityClass::loop()
{
}

void WebApiSecurityClass::onSecurityGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("password")] = config.Security_Password;
    root[F("allow_readonly")] = config.Security_AllowReadonly;

    response->setLength();
    request->send(response);
}

void WebApiSecurityClass::onSecurityPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        response->setLength();
        request->send(response);
        return;
    }

    if (!root.containsKey("password")
        && root.containsKey("allow_readonly")) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("password")].as<String>().length() < 8 || root[F("password")].as<String>().length() > WIFI_MAX_PASSWORD_STRLEN) {
        retMsg[F("message")] = F("Password must between 8 and " STR(WIFI_MAX_PASSWORD_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strlcpy(config.Security_Password, root[F("password")].as<String>().c_str(), sizeof(config.Security_Password));
    config.Security_AllowReadonly = root[F("allow_readonly")].as<bool>();
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);
}

void WebApiSecurityClass::onAuthenticateGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Authentication successfull!");

    response->setLength();
    request->send(response);
}
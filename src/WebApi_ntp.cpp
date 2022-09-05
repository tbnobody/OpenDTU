// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ntp.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "helper.h"
#include "NtpSettings.h"

void WebApiNtpClass::init(AsyncWebServer* server)
{
    using namespace std::placeholders;

    _server = server;

    _server->on("/api/ntp/status", HTTP_GET, std::bind(&WebApiNtpClass::onNtpStatus, this, _1));
    _server->on("/api/ntp/config", HTTP_GET, std::bind(&WebApiNtpClass::onNtpAdminGet, this, _1));
    _server->on("/api/ntp/config", HTTP_POST, std::bind(&WebApiNtpClass::onNtpAdminPost, this, _1));
}

void WebApiNtpClass::loop()
{
}

void WebApiNtpClass::onNtpStatus(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("ntp_server")] = config.Ntp_Server;
    root[F("ntp_timezone")] = config.Ntp_Timezone;
    root[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        root[F("ntp_status")] = false;
    } else {
        root[F("ntp_status")] = true;
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    root[F("ntp_localtime")] = timeStringBuff;

    response->setLength();
    request->send(response);
}

void WebApiNtpClass::onNtpAdminGet(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("ntp_server")] = config.Ntp_Server;
    root[F("ntp_timezone")] = config.Ntp_Timezone;
    root[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    response->setLength();
    request->send(response);
}

void WebApiNtpClass::onNtpAdminPost(AsyncWebServerRequest* request)
{
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

    if (!(root.containsKey("ntp_server") && root.containsKey("ntp_timezone"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_server")].as<String>().length() == 0 || root[F("ntp_server")].as<String>().length() > NTP_MAX_SERVER_STRLEN) {
        retMsg[F("message")] = F("NTP Server must between 1 and " STR(NTP_MAX_SERVER_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone")].as<String>().length() == 0 || root[F("ntp_timezone")].as<String>().length() > NTP_MAX_TIMEZONE_STRLEN) {
        retMsg[F("message")] = F("Timezone must between 1 and " STR(NTP_MAX_TIMEZONE_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone_descr")].as<String>().length() == 0 || root[F("ntp_timezone_descr")].as<String>().length() > NTP_MAX_TIMEZONEDESCR_STRLEN) {
        retMsg[F("message")] = F("Timezone description must between 1 and " STR(NTP_MAX_TIMEZONEDESCR_STRLEN) " characters long!");
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strcpy(config.Ntp_Server, root[F("ntp_server")].as<String>().c_str());
    strcpy(config.Ntp_Timezone, root[F("ntp_timezone")].as<String>().c_str());
    strcpy(config.Ntp_TimezoneDescr, root[F("ntp_timezone_descr")].as<String>().c_str());
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");

    response->setLength();
    request->send(response);

    NtpSettings.setServer();
    NtpSettings.setTimezone();
}
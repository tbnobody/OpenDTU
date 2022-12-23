// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_ntp.h"
#include "Configuration.h"
#include "NtpSettings.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiNtpClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/ntp/status", HTTP_GET, std::bind(&WebApiNtpClass::onNtpStatus, this, _1));
    _server->on("/api/ntp/config", HTTP_GET, std::bind(&WebApiNtpClass::onNtpAdminGet, this, _1));
    _server->on("/api/ntp/config", HTTP_POST, std::bind(&WebApiNtpClass::onNtpAdminPost, this, _1));
    _server->on("/api/ntp/time", HTTP_GET, std::bind(&WebApiNtpClass::onNtpTimeGet, this, _1));
    _server->on("/api/ntp/time", HTTP_POST, std::bind(&WebApiNtpClass::onNtpTimePost, this, _1));
}

void WebApiNtpClass::loop()
{
}

void WebApiNtpClass::onNtpStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root[F("ntp_server")] = config.Ntp_Server;
    root[F("ntp_timezone")] = config.Ntp_Timezone;
    root[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
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
    if (!WebApi.checkCredentials(request)) {
        return;
    }

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
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("ntp_server") && root.containsKey("ntp_timezone"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_server")].as<String>().length() == 0 || root[F("ntp_server")].as<String>().length() > NTP_MAX_SERVER_STRLEN) {
        retMsg[F("message")] = F("NTP Server must between 1 and " STR(NTP_MAX_SERVER_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::NtpServerLength;
        retMsg[F("param")][F("max")] = NTP_MAX_SERVER_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone")].as<String>().length() == 0 || root[F("ntp_timezone")].as<String>().length() > NTP_MAX_TIMEZONE_STRLEN) {
        retMsg[F("message")] = F("Timezone must between 1 and " STR(NTP_MAX_TIMEZONE_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::NtpTimezoneLength;
        retMsg[F("param")][F("max")] = NTP_MAX_TIMEZONE_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("ntp_timezone_descr")].as<String>().length() == 0 || root[F("ntp_timezone_descr")].as<String>().length() > NTP_MAX_TIMEZONEDESCR_STRLEN) {
        retMsg[F("message")] = F("Timezone description must between 1 and " STR(NTP_MAX_TIMEZONEDESCR_STRLEN) " characters long!");
        retMsg[F("code")] = WebApiError::NtpTimezoneDescriptionLength;
        retMsg[F("param")][F("max")] = NTP_MAX_TIMEZONEDESCR_STRLEN;
        response->setLength();
        request->send(response);
        return;
    }

    CONFIG_T& config = Configuration.get();
    strlcpy(config.Ntp_Server, root[F("ntp_server")].as<String>().c_str(), sizeof(config.Ntp_Server));
    strlcpy(config.Ntp_Timezone, root[F("ntp_timezone")].as<String>().c_str(), sizeof(config.Ntp_Timezone));
    strlcpy(config.Ntp_TimezoneDescr, root[F("ntp_timezone_descr")].as<String>().c_str(), sizeof(config.Ntp_TimezoneDescr));
    Configuration.write();

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Settings saved!");
    retMsg[F("code")] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);

    NtpSettings.setServer();
    NtpSettings.setTimezone();
}

void WebApiNtpClass::onNtpTimeGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        root[F("ntp_status")] = false;
    } else {
        root[F("ntp_status")] = true;
    }

    root[F("year")] = timeinfo.tm_year + 1900;
    root[F("month")] = timeinfo.tm_mon + 1;
    root[F("day")] = timeinfo.tm_mday;
    root[F("hour")] = timeinfo.tm_hour;
    root[F("minute")] = timeinfo.tm_min;
    root[F("second")] = timeinfo.tm_sec;

    response->setLength();
    request->send(response);
}

void WebApiNtpClass::onNtpTimePost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg[F("type")] = F("warning");

    if (!request->hasParam("data", true)) {
        retMsg[F("message")] = F("No values found!");
        retMsg[F("code")] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg[F("message")] = F("Data too large!");
        retMsg[F("code")] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg[F("message")] = F("Failed to parse data!");
        retMsg[F("code")] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("year")
            && root.containsKey("month")
            && root.containsKey("day")
            && root.containsKey("hour")
            && root.containsKey("minute")
            && root.containsKey("second"))) {
        retMsg[F("message")] = F("Values are missing!");
        retMsg[F("code")] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("year")].as<uint>() < 2022 || root[F("year")].as<uint>() > 2100) {
        retMsg[F("message")] = F("Year must be a number between 2022 and 2100!");
        retMsg[F("code")] = WebApiError::NtpYearInvalid;
        retMsg[F("param")][F("min")] = 2022;
        retMsg[F("param")][F("max")] = 2100;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("month")].as<uint>() < 1 || root[F("month")].as<uint>() > 12) {
        retMsg[F("message")] = F("Month must be a number between 1 and 12!");
        retMsg[F("code")] = WebApiError::NtpMonthInvalid;
        retMsg[F("param")][F("min")] = 1;
        retMsg[F("param")][F("max")] = 12;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("day")].as<uint>() < 1 || root[F("day")].as<uint>() > 31) {
        retMsg[F("message")] = F("Day must be a number between 1 and 31!");
        retMsg[F("code")] = WebApiError::NtpDayInvalid;
        retMsg[F("param")][F("min")] = 1;
        retMsg[F("param")][F("max")] = 31;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("hour")].as<uint>() > 23) {
        retMsg[F("message")] = F("Hour must be a number between 0 and 23!");
        retMsg[F("code")] = WebApiError::NtpHourInvalid;
        retMsg[F("param")][F("min")] = 0;
        retMsg[F("param")][F("max")] = 23;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("minute")].as<uint>() > 59) {
        retMsg[F("message")] = F("Minute must be a number between 0 and 59!");
        retMsg[F("code")] = WebApiError::NtpMinuteInvalid;
        retMsg[F("param")][F("min")] = 0;
        retMsg[F("param")][F("max")] = 59;
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("second")].as<uint>() > 59) {
        retMsg[F("message")] = F("Second must be a number between 0 and 59!");
        retMsg[F("code")] = WebApiError::NtpSecondInvalid;
        retMsg[F("param")][F("min")] = 0;
        retMsg[F("param")][F("max")] = 59;
        response->setLength();
        request->send(response);
        return;
    }

    tm local;
    local.tm_sec = root[F("second")].as<uint>(); // seconds after the minute - [ 0 to 59 ]
    local.tm_min = root[F("minute")].as<uint>(); // minutes after the hour - [ 0 to 59 ]
    local.tm_hour = root[F("hour")].as<uint>(); // hours since midnight - [ 0 to 23 ]
    local.tm_mday = root[F("day")].as<uint>(); // day of the month - [ 1 to 31 ]
    local.tm_mon = root[F("month")].as<uint>() - 1; // months since January - [ 0 to 11 ]
    local.tm_year = root[F("year")].as<uint>() - 1900; // years since 1900
    local.tm_isdst = -1;

    time_t t = mktime(&local);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Time updated!");
    retMsg[F("code")] = WebApiError::NtpTimeUpdated;

    response->setLength();
    request->send(response);
}
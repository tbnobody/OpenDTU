// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_ntp.h"
#include "Configuration.h"
#include "NtpSettings.h"
#include "SunPosition.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiNtpClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    server.on("/api/ntp/status", HTTP_GET, std::bind(&WebApiNtpClass::onNtpStatus, this, _1));
    server.on("/api/ntp/config", HTTP_GET, std::bind(&WebApiNtpClass::onNtpAdminGet, this, _1));
    server.on("/api/ntp/config", HTTP_POST, std::bind(&WebApiNtpClass::onNtpAdminPost, this, _1));
    server.on("/api/ntp/time", HTTP_GET, std::bind(&WebApiNtpClass::onNtpTimeGet, this, _1));
    server.on("/api/ntp/time", HTTP_POST, std::bind(&WebApiNtpClass::onNtpTimePost, this, _1));
}

void WebApiNtpClass::onNtpStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["ntp_server"] = config.Ntp.Server;
    root["ntp_timezone"] = config.Ntp.Timezone;
    root["ntp_timezone_descr"] = config.Ntp.TimezoneDescr;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        root["ntp_status"] = false;
    } else {
        root["ntp_status"] = true;
    }
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    root["ntp_localtime"] = timeStringBuff;

    if (SunPosition.sunriseTime(&timeinfo)) {
        strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    } else {
        snprintf(timeStringBuff, sizeof(timeStringBuff), "--");
    }
    root["sun_risetime"] = timeStringBuff;

    if (SunPosition.sunsetTime(&timeinfo)) {
        strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    } else {
        snprintf(timeStringBuff, sizeof(timeStringBuff), "--");
    }
    root["sun_settime"] = timeStringBuff;

    root["sun_isSunsetAvailable"] = SunPosition.isSunsetAvailable();
    root["sun_isDayPeriod"] = SunPosition.isDayPeriod();

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiNtpClass::onNtpAdminGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    const CONFIG_T& config = Configuration.get();

    root["ntp_server"] = config.Ntp.Server;
    root["ntp_timezone"] = config.Ntp.Timezone;
    root["ntp_timezone_descr"] = config.Ntp.TimezoneDescr;
    root["longitude"] = config.Ntp.Longitude;
    root["latitude"] = config.Ntp.Latitude;
    root["sunsettype"] = config.Ntp.SunsetType;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiNtpClass::onNtpAdminPost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["ntp_server"].is<String>()
            && root["ntp_timezone"].is<String>()
            && root["longitude"].is<double>()
            && root["latitude"].is<double>()
            && root["sunsettype"].is<uint8_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["ntp_server"].as<String>().length() == 0 || root["ntp_server"].as<String>().length() > NTP_MAX_SERVER_STRLEN) {
        retMsg["message"] = "NTP Server must between 1 and " STR(NTP_MAX_SERVER_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::NtpServerLength;
        retMsg["param"]["max"] = NTP_MAX_SERVER_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["ntp_timezone"].as<String>().length() == 0 || root["ntp_timezone"].as<String>().length() > NTP_MAX_TIMEZONE_STRLEN) {
        retMsg["message"] = "Timezone must between 1 and " STR(NTP_MAX_TIMEZONE_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::NtpTimezoneLength;
        retMsg["param"]["max"] = NTP_MAX_TIMEZONE_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["ntp_timezone_descr"].as<String>().length() == 0 || root["ntp_timezone_descr"].as<String>().length() > NTP_MAX_TIMEZONEDESCR_STRLEN) {
        retMsg["message"] = "Timezone description must between 1 and " STR(NTP_MAX_TIMEZONEDESCR_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::NtpTimezoneDescriptionLength;
        retMsg["param"]["max"] = NTP_MAX_TIMEZONEDESCR_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        strlcpy(config.Ntp.Server, root["ntp_server"].as<String>().c_str(), sizeof(config.Ntp.Server));
        strlcpy(config.Ntp.Timezone, root["ntp_timezone"].as<String>().c_str(), sizeof(config.Ntp.Timezone));
        strlcpy(config.Ntp.TimezoneDescr, root["ntp_timezone_descr"].as<String>().c_str(), sizeof(config.Ntp.TimezoneDescr));
        config.Ntp.Latitude = root["latitude"].as<double>();
        config.Ntp.Longitude = root["longitude"].as<double>();
        config.Ntp.SunsetType = root["sunsettype"].as<uint8_t>();
    }

    WebApi.writeConfig(retMsg);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    NtpSettings.setServer();
    NtpSettings.setTimezone();

    SunPosition.setDoRecalc(true);
}

void WebApiNtpClass::onNtpTimeGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        root["ntp_status"] = false;
    } else {
        root["ntp_status"] = true;
    }

    root["year"] = timeinfo.tm_year + 1900;
    root["month"] = timeinfo.tm_mon + 1;
    root["day"] = timeinfo.tm_mday;
    root["hour"] = timeinfo.tm_hour;
    root["minute"] = timeinfo.tm_min;
    root["second"] = timeinfo.tm_sec;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiNtpClass::onNtpTimePost(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["year"].is<uint>()
            && root["month"].is<uint>()
            && root["day"].is<uint>()
            && root["hour"].is<uint>()
            && root["minute"].is<uint>()
            && root["second"].is<uint>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["year"].as<uint>() < 2022 || root["year"].as<uint>() > 2100) {
        retMsg["message"] = "Year must be a number between 2022 and 2100!";
        retMsg["code"] = WebApiError::NtpYearInvalid;
        retMsg["param"]["min"] = 2022;
        retMsg["param"]["max"] = 2100;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["month"].as<uint>() < 1 || root["month"].as<uint>() > 12) {
        retMsg["message"] = "Month must be a number between 1 and 12!";
        retMsg["code"] = WebApiError::NtpMonthInvalid;
        retMsg["param"]["min"] = 1;
        retMsg["param"]["max"] = 12;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["day"].as<uint>() < 1 || root["day"].as<uint>() > 31) {
        retMsg["message"] = "Day must be a number between 1 and 31!";
        retMsg["code"] = WebApiError::NtpDayInvalid;
        retMsg["param"]["min"] = 1;
        retMsg["param"]["max"] = 31;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["hour"].as<uint>() > 23) {
        retMsg["message"] = "Hour must be a number between 0 and 23!";
        retMsg["code"] = WebApiError::NtpHourInvalid;
        retMsg["param"]["min"] = 0;
        retMsg["param"]["max"] = 23;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["minute"].as<uint>() > 59) {
        retMsg["message"] = "Minute must be a number between 0 and 59!";
        retMsg["code"] = WebApiError::NtpMinuteInvalid;
        retMsg["param"]["min"] = 0;
        retMsg["param"]["max"] = 59;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["second"].as<uint>() > 59) {
        retMsg["message"] = "Second must be a number between 0 and 59!";
        retMsg["code"] = WebApiError::NtpSecondInvalid;
        retMsg["param"]["min"] = 0;
        retMsg["param"]["max"] = 59;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    tm local;
    local.tm_sec = root["second"].as<uint>(); // seconds after the minute - [ 0 to 59 ]
    local.tm_min = root["minute"].as<uint>(); // minutes after the hour - [ 0 to 59 ]
    local.tm_hour = root["hour"].as<uint>(); // hours since midnight - [ 0 to 23 ]
    local.tm_mday = root["day"].as<uint>(); // day of the month - [ 1 to 31 ]
    local.tm_mon = root["month"].as<uint>() - 1; // months since January - [ 0 to 11 ]
    local.tm_year = root["year"].as<uint>() - 1900; // years since 1900
    local.tm_isdst = -1;

    time_t t = mktime(&local);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);

    retMsg["type"] = "success";
    retMsg["message"] = "Time updated!";
    retMsg["code"] = WebApiError::NtpTimeUpdated;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

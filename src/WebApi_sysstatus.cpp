// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_sysstatus.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include "WebApi.h"
#include <AsyncJson.h>
#include <Hoymiles.h>
#include <LittleFS.h>
#include <ResetReason.h>

#ifndef AUTO_GIT_HASH
#define AUTO_GIT_HASH ""
#endif

void WebApiSysstatusClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/system/status", HTTP_GET, std::bind(&WebApiSysstatusClass::onSystemStatus, this, _1));
}

void WebApiSysstatusClass::loop()
{
}

void WebApiSysstatusClass::onSystemStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();

    root[F("hostname")] = NetworkSettings.getHostname();

    root[F("sdkversion")] = ESP.getSdkVersion();
    root[F("cpufreq")] = ESP.getCpuFreqMHz();

    root[F("heap_total")] = ESP.getHeapSize();
    root[F("heap_used")] = ESP.getHeapSize() - ESP.getFreeHeap();
    root[F("sketch_total")] = ESP.getFreeSketchSpace();
    root[F("sketch_used")] = ESP.getSketchSize();
    root[F("littlefs_total")] = LittleFS.totalBytes();
    root[F("littlefs_used")] = LittleFS.usedBytes();

    root[F("chiprevision")] = ESP.getChipRevision();
    root[F("chipmodel")] = ESP.getChipModel();
    root[F("chipcores")] = ESP.getChipCores();

    String reason;
    reason = ResetReason.get_reset_reason_verbose(0);
    root[F("resetreason_0")] = reason;

    reason = ResetReason.get_reset_reason_verbose(1);
    root[F("resetreason_1")] = reason;

    root[F("cfgsavecount")] = Configuration.get().Cfg_SaveCount;

    char version[16];
    snprintf(version, sizeof(version), "%d.%d.%d", CONFIG_VERSION >> 24 & 0xff, CONFIG_VERSION >> 16 & 0xff, CONFIG_VERSION >> 8 & 0xff);
    root[F("config_version")] = version;
    root[F("git_hash")] = AUTO_GIT_HASH;

    root[F("uptime")] = esp_timer_get_time() / 1000000;

    root[F("radio_connected")] = Hoymiles.getRadio()->isConnected();
    root[F("radio_pvariant")] = Hoymiles.getRadio()->isPVariant();

    response->setLength();
    request->send(response);
}
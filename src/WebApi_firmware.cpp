// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_firmware.h"
#include "Configuration.h"
#include "Update.h"
#include "Utils.h"
#include "WebApi.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiFirmwareClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    server.on("/api/firmware/update", HTTP_POST,
        std::bind(&WebApiFirmwareClass::onFirmwareUpdateFinish, this, _1),
        std::bind(&WebApiFirmwareClass::onFirmwareUpdateUpload, this, _1, _2, _3, _4, _5, _6));
}

void WebApiFirmwareClass::onFirmwareUpdateFinish(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response

    AsyncWebServerResponse* response = request->beginResponse((Update.hasError()) ? 500 : 200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    Utils::restartDtu();
}

void WebApiFirmwareClass::onFirmwareUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    // Upload handler chunks in data
    if (!index) {
        if (!request->hasParam("MD5", true)) {
            return request->send(400, "text/plain", "MD5 parameter missing");
        }

        if (!Update.setMD5(request->getParam("MD5", true)->value().c_str())) {
            return request->send(400, "text/plain", "MD5 parameter invalid");
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) { // Start with max available size
            Update.printError(Serial);
            return request->send(400, "text/plain", "OTA could not begin");
        }
    }

    // Write chunked data to the free sketch space
    if (len) {
        if (Update.write(data, len) != len) {
            return request->send(400, "text/plain", "OTA could not begin");
        }
    }

    if (final) { // if the final flag is set then this is the last frame of data
        if (!Update.end(true)) { // true to set the size to the current progress
            Update.printError(Serial);
            return request->send(400, "text/plain", "Could not end OTA");
        }
    } else {
        return;
    }
}

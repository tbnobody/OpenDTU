// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_config.h"
#include "Configuration.h"
#include "Utils.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiConfigClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = &server;

    _server->on("/api/config/get", HTTP_GET, std::bind(&WebApiConfigClass::onConfigGet, this, _1));
    _server->on("/api/config/delete", HTTP_POST, std::bind(&WebApiConfigClass::onConfigDelete, this, _1));
    _server->on("/api/config/list", HTTP_GET, std::bind(&WebApiConfigClass::onConfigListGet, this, _1));
    _server->on("/api/config/upload", HTTP_POST,
        std::bind(&WebApiConfigClass::onConfigUploadFinish, this, _1),
        std::bind(&WebApiConfigClass::onConfigUpload, this, _1, _2, _3, _4, _5, _6));
}

void WebApiConfigClass::onConfigGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    String requestFile = CONFIG_FILENAME;
    if (request->hasParam("file")) {
        String name = "/" + request->getParam("file")->value();
        if (LittleFS.exists(name)) {
            requestFile = name;
        } else {
            request->send(404);
        }
    }

    request->send(LittleFS, requestFile, String(), true);
}

void WebApiConfigClass::onConfigDelete(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& retMsg = response->getRoot();
    retMsg["type"] = "warning";

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    const String json = request->getParam("data", true)->value();

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument root(1024);
    const DeserializationError error = deserializeJson(root, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    if (!(root.containsKey("delete"))) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    if (root["delete"].as<bool>() == false) {
        retMsg["message"] = "Not deleted anything!";
        retMsg["code"] = WebApiError::ConfigNotDeleted;
        response->setLength();
        request->send(response);
        return;
    }

    retMsg["type"] = "success";
    retMsg["message"] = "Configuration resettet. Rebooting now...";
    retMsg["code"] = WebApiError::ConfigSuccess;

    response->setLength();
    request->send(response);

    LittleFS.remove(CONFIG_FILENAME);
    Utils::restartDtu();
}

void WebApiConfigClass::onConfigListGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto data = root.createNestedArray("configs");

    File rootfs = LittleFS.open("/");
    File file = rootfs.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            continue;
        }
        JsonObject obj = data.createNestedObject();
        obj["name"] = String(file.name());

        file = rootfs.openNextFile();
    }
    file.close();

    response->setLength();
    request->send(response);
}

void WebApiConfigClass::onConfigUploadFinish(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    // the request handler is triggered after the upload has finished...
    // create the response, add header, and send response

    AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "OK");
    response->addHeader("Connection", "close");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
    Utils::restartDtu();
}

void WebApiConfigClass::onConfigUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    if (!index) {
        // open the file on first call and store the file handle in the request object
        if (!request->hasParam("file")) {
            request->send(500);
            return;
        }
        const String name = "/" + request->getParam("file")->value();
        request->_tempFile = LittleFS.open(name, "w");
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
    }

    if (final) {
        // close the file handle as the upload is now done
        request->_tempFile.close();
    }
}

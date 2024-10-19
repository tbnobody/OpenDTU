// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "WebApi_file.h"
#include "Configuration.h"
#include "RestartHelper.h"
#include "Utils.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <LittleFS.h>

void WebApiFileClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    server.on("/api/file/get", HTTP_GET, std::bind(&WebApiFileClass::onFileGet, this, _1));
    server.on("/api/file/delete", HTTP_POST, std::bind(&WebApiFileClass::onFileDelete, this, _1));
    server.on("/api/file/delete_all", HTTP_POST, std::bind(&WebApiFileClass::onFileDeleteAll, this, _1));
    server.on("/api/file/list", HTTP_GET, std::bind(&WebApiFileClass::onFileListGet, this, _1));
    server.on("/api/file/upload", HTTP_POST,
        std::bind(&WebApiFileClass::onFileUploadFinish, this, _1),
        std::bind(&WebApiFileClass::onFileUpload, this, _1, _2, _3, _4, _5, _6));
}

void WebApiFileClass::onFileListGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    auto data = root.to<JsonArray>();

    File rootfs = LittleFS.open("/");
    File file = rootfs.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            continue;
        }
        JsonObject obj = data.add<JsonObject>();
        obj["name"] = String(file.name());
        obj["size"] = file.size();

        file = rootfs.openNextFile();
    }
    file.close();

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiFileClass::onFileGet(AsyncWebServerRequest* request)
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
            return;
        }
    }

    request->send(LittleFS, requestFile, String(), true);
}

void WebApiFileClass::onFileDelete(AsyncWebServerRequest* request)
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

    if (!(root["file"].is<String>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    String name = "/" + root["file"].as<String>();
    if (!LittleFS.exists(name)) {
        request->send(404);
        return;
    }

    LittleFS.remove(name);

    retMsg["type"] = "success";
    retMsg["message"] = "File deleted";
    retMsg["code"] = WebApiError::FileDeleteSuccess;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiFileClass::onFileDeleteAll(AsyncWebServerRequest* request)
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

    if (!(root["delete"].is<bool>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["delete"].as<bool>() == false) {
        retMsg["message"] = "Not deleted anything!";
        retMsg["code"] = WebApiError::FileNotDeleted;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    retMsg["type"] = "success";
    retMsg["message"] = "Configuration resettet. Rebooting now...";
    retMsg["code"] = WebApiError::FileSuccess;

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    Utils::removeAllFiles();
    RestartHelper.triggerRestart();
}

void WebApiFileClass::onFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
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

void WebApiFileClass::onFileUploadFinish(AsyncWebServerRequest* request)
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
    RestartHelper.triggerRestart();
}

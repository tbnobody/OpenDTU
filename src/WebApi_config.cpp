// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_config.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Configuration.h"
#include "WebApi.h"
#include <LittleFS.h>

void WebApiConfigClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    _server = server;

    _server->on("/api/config/get", HTTP_GET, std::bind(&WebApiConfigClass::onConfigGet, this, _1));
    _server->on("/api/config/delete", HTTP_POST, std::bind(&WebApiConfigClass::onConfigDelete, this, _1));
    _server->on("/api/config/upload", HTTP_POST,
        std::bind(&WebApiConfigClass::onConfigUploadFinish, this, _1),
        std::bind(&WebApiConfigClass::onConfigUpload, this, _1, _2, _3, _4, _5, _6));
}

void WebApiConfigClass::loop()
{
}

void WebApiConfigClass::onConfigGet(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    request->send(LittleFS, CONFIG_FILENAME_JSON, String(), true);
}

void WebApiConfigClass::onConfigDelete(AsyncWebServerRequest* request)
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

    if (!(root.containsKey("delete"))) {
        retMsg[F("message")] = F("Values are missing!");
        response->setLength();
        request->send(response);
        return;
    }

    if (root[F("delete")].as<bool>() == false) {
        retMsg[F("message")] = F("Not deleted anything!");
        response->setLength();
        request->send(response);
        return;
    }

    retMsg[F("type")] = F("success");
    retMsg[F("message")] = F("Configuration resettet. Rebooting now...");

    response->setLength();
    request->send(response);

    LittleFS.remove(CONFIG_FILENAME_JSON);
    ESP.restart();
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
    yield();
    delay(1000);
    yield();
    ESP.restart();
}

void WebApiConfigClass::onConfigUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    if (!index) {
        // open the file on first call and store the file handle in the request object
        request->_tempFile = LittleFS.open(CONFIG_FILENAME_JSON, "w");
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
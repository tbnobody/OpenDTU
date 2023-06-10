// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_plugins.h"
#include "MessageOutput.h"
#include "MqttHandleHass.h"
#include "PluginConfiguration.h"
#include "Plugins.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "helper.h"
#include <AsyncJson.h>

void WebApiPluginsClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/plugin/list", HTTP_GET, std::bind(&WebApiPluginsClass::onPluginList, this, _1));
    // _server->on("/api/inverter/add", HTTP_POST, std::bind(&WebApiPluginsClass::onInverterAdd, this, _1));
    _server->on("/api/plugin/edit", HTTP_POST, std::bind(&WebApiPluginsClass::onPluginEdit, this, _1));
    // _server->on("/api/inverter/del", HTTP_POST, std::bind(&WebApiPluginsClass::onInverterDelete, this, _1));
    _server->on("/api/plugin/request", HTTP_ANY, std::bind(&WebApiPluginsClass::onPluginRequest, this, _1));

}

void WebApiPluginsClass::loop()
{
}

void WebApiPluginsClass::onPluginRequest(AsyncWebServerRequest* request)
{
    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";
    retMsg["code"] = WebApiError::GenericBase;

    for(int i = 0 ; i < request->params() ; i++) {
        MessageOutput.printf("meter %s\n",request->getParam(i)->name().c_str());
    }
    if (!request->hasParam("body",true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("body",true)->value();
    MessageOutput.printf("got data (%d): %s\n",json.length(), json.c_str());

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }
    JsonObject root = doc.as<JsonObject>();
    Plugin* p = Plugins.getPluginByName(root["plugin"]);
    if (p == NULL) {
        retMsg["message"] = "Plugin not found!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }
    retMsg["type"] = "success";
    retMsg["code"] = WebApiError::GenericSuccess;
    retMsg["ok"]=p->onRequest(root,retMsg);
    response->setLength();
    request->send(response);
}

void WebApiPluginsClass::onPluginList(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }
    AsyncJsonResponse* response = new AsyncJsonResponse(false, 4096U);
    JsonObject root = response->getRoot();
    JsonArray plugins = root.createNestedArray(F("plugins"));
    for (int i = 0; i < Plugins.getPluginCount(); i++) {
        Plugin* p = Plugins.getPluginByIndex(i);
        JsonObject pjson = plugins.createNestedObject();
        pjson[F("name")] = String(p->name);
        PluginConfiguration.writeTo(pjson, p);
    }
    response->setLength();
    request->send(response);
}

void WebApiPluginsClass::onPluginEdit(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";
    retMsg["code"] = WebApiError::GenericBase;

    if (!request->hasParam("data", true)) {
        retMsg["message"] = "No values found!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        response->setLength();
        request->send(response);
        return;
    }

    String json = request->getParam("data", true)->value();
    MessageOutput.printf("got data (%d): %s\n",json.length(), json.c_str());

    if (json.length() > 1024) {
        retMsg["message"] = "Data too large!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        response->setLength();
        request->send(response);
        return;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        retMsg["message"] = "Failed to parse data!";
        retMsg["code"] = WebApiError::GenericParseError;
        response->setLength();
        request->send(response);
        return;
    }
    JsonObject root = doc.as<JsonObject>();

    if (!root.containsKey("name")) {
        retMsg["message"] = "Plugin name not set!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }
    String pluginname = root["name"];
    Plugin* p = Plugins.getPluginByName(pluginname.c_str());
    if (p == NULL) {
        retMsg["message"] = "Plugin not found!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }
    p->loadPluginSettings(root);
    p->loadSettings(root);
    if (PluginConfiguration.write(p)) {
        retMsg["type"] = "success";
        retMsg["message"] = "Settings saved!";
        retMsg["code"] = WebApiError::GenericSuccess;
    }

    response->setLength();
    request->send(response);
}

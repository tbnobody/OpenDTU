// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_Pylontech.h"
#include "Battery.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include <AsyncJson.h>
#include <Hoymiles.h>

void WebApiPylontechClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->on("/api/battery/livedata", HTTP_GET, std::bind(&WebApiPylontechClass::onStatus, this, _1));
}

void WebApiPylontechClass::loop()
{
}

void WebApiPylontechClass::getJsonData(JsonObject& root) {

    root["data_age"] = (millis() - Battery.lastUpdate) / 1000;

    root[F("chargeVoltage")]["v"] = Battery.chargeVoltage ;
    root[F("chargeVoltage")]["u"] = "V";
    root[F("chargeCurrentLimitation")]["v"] = Battery.chargeCurrentLimitation ;
    root[F("chargeCurrentLimitation")]["u"] = "A";
    root[F("dischargeCurrentLimitation")]["v"] = Battery.dischargeCurrentLimitation ;
    root[F("dischargeCurrentLimitation")]["u"] = "A";
    root[F("stateOfCharge")]["v"] = Battery.stateOfCharge ;
    root[F("stateOfCharge")]["u"] = "%";
    root[F("stateOfHealth")]["v"] = Battery.stateOfHealth ;
    root[F("stateOfHealth")]["u"] = "%";
    root[F("voltage")]["v"] = Battery.voltage;
    root[F("voltage")]["u"] = "V";
    root[F("current")]["v"] = Battery.current ;
    root[F("current")]["u"] = "A";
    root[F("temperature")]["v"] = Battery.temperature ;
    root[F("temperature")]["u"] = "°C";

    // Alarms
    root["alarms"][F("dischargeCurrent")] = Battery.alarmOverCurrentDischarge ;
    root["alarms"][F("chargeCurrent")] = Battery.alarmOverCurrentCharge ;
    root["alarms"][F("lowTemperature")] = Battery.alarmUnderTemperature ;
    root["alarms"][F("highTemperature")] = Battery.alarmOverTemperature ;
    root["alarms"][F("lowVoltage")] = Battery.alarmUnderVoltage ;
    root["alarms"][F("highVoltage")] = Battery.alarmOverVoltage ;
    root["alarms"][F("bmsInternal")] = Battery.alarmBmsInternal ;

    // Warnings
    root["warnings"][F("dischargeCurrent")] = Battery.warningHighCurrentDischarge ;
    root["warnings"][F("chargeCurrent")] = Battery.warningHighCurrentCharge ;
    root["warnings"][F("lowTemperature")] = Battery.warningLowTemperature ;
    root["warnings"][F("highTemperature")] = Battery.warningHighTemperature ;
    root["warnings"][F("lowVoltage")] = Battery.warningLowVoltage ;
    root["warnings"][F("highVoltage")] = Battery.warningHighVoltage ;
    root["warnings"][F("bmsInternal")] = Battery.warningBmsInternal ;

    // Misc
    root[F("manufacturer")] = Battery.manufacturer ;
    root[F("chargeEnabled")] = Battery.chargeEnabled ;
    root[F("dischargeEnabled")] = Battery.dischargeEnabled ;
    root[F("chargeImmediately")] = Battery.chargeImmediately ;

}

void WebApiPylontechClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject root = response->getRoot();
    getJsonData(root);

    response->setLength();
    request->send(response);
}


// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_safe_limit.h"
#include "WebApi.h"
#include "WebApi_errors.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include <AsyncJson.h>
#include <Hoymiles.h>
#include <Arduino.h>

static void setLimit(uint64_t inverterSerial, uint16_t watts, const char *cause)
{
    auto inv = Hoymiles.getInverterBySerial(inverterSerial);
    if (inv != 0) {
	MessageOutput.print("Inverter ");
	MessageOutput.print(inverterSerial, HEX);
	MessageOutput.printf(" new limit is %uW (%s)\r\n", watts, cause);
	inv->sendActivePowerControlRequest(watts, PowerLimitControlType::AbsolutNonPersistent);
    } else {
        MessageOutput.print("Ignored safe limit for mising inverter ");
	MessageOutput.print(inverterSerial, HEX);
	MessageOutput.print("\r\n");
    }
}

void WebApiSafeLimitClass::init(AsyncWebServer *server)
{
    for (auto &conf : Configuration.get().Inverter) {
        if (conf.SafeLimitMillis != 0) {
            setLimit(conf.Serial, conf.SafeLimitWatts, "init");
        }
    }

    using std::placeholders::_1;
    _server = server;
    _server->on("/api/limit/safe", HTTP_POST, std::bind(&WebApiSafeLimitClass::onSafeLimitPost, this, _1));
}

void WebApiSafeLimitClass::loop()
{
    int32_t millisNow = (int32_t)millis();
    for (auto &fallback : _fallback) {
        if (fallback._inverterSerial != 0 && millisNow - fallback._timeoutMillis > 0) {
            if (auto conf = Configuration.getInverterConfig(fallback._inverterSerial)) {
                setLimit(fallback._inverterSerial, conf->SafeLimitWatts, "timeout");
            }
            fallback._inverterSerial = 0;
        }
    }
}

void WebApiSafeLimitClass::onSafeLimitPost(AsyncWebServerRequest *request) {
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonObject retMsg = response->getRoot();
    retMsg["type"] = "warning";

    auto serialParam = request->getParam("serial", true);
    auto limitParam = request->getParam("limit", true);
    if (serialParam == nullptr || limitParam == nullptr) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        response->setLength();
        request->send(response);
        return;
    }

    uint64_t serial = strtoll(serialParam->value().c_str(), NULL, 16);
    if (serial == 0) {
        retMsg["message"] = "Serial must be a number > 0!";
        retMsg["code"] = WebApiError::LimitSerialZero;
        response->setLength();
        request->send(response);
        return;
    }

    auto limit = limitParam->value().toInt();
    if (limit == 0 || limit > 2250) {
        retMsg["message"] = "Limit must between 1 and 2250!";
        retMsg["code"] = WebApiError::LimitInvalidLimit;
        retMsg["param"]["max"] = 2250;
        response->setLength();
        request->send(response);
        return;
    }

    auto inv = Hoymiles.getInverterBySerial(serial);
    if (inv == nullptr) {
        retMsg["message"] = "Invalid inverter specified!";
        retMsg["code"] = WebApiError::LimitInvalidInverter;
        response->setLength();
        request->send(response);
        return;
    }

    auto conf = Configuration.getInverterConfig(serial);
    if (conf == 0 || conf->SafeLimitMillis == 0) {
        retMsg["message"] = "Unconfigured inverter specified!";
        retMsg["code"] = WebApiError::LimitInvalidInverter;
        response->setLength();
        request->send(response);
        return;
    }

    auto fallback = getFallback(serial);
    if (fallback == 0) {
        retMsg["message"] = "No empty inverter slot in safe limit state!";
        retMsg["code"] = WebApiError::LimitInvalidInverter;
        response->setLength();
        request->send(response);
        return;
    }

    if (fallback->_currentLimit != limit) {
        setLimit(serial, limit, "update");
        fallback->_currentLimit = limit;
    }
    fallback->_timeoutMillis = millis() + conf->SafeLimitMillis;

    retMsg["type"] = "success";
    retMsg["message"] = "Settings saved!";
    retMsg["code"] = WebApiError::GenericSuccess;

    response->setLength();
    request->send(response);
}

WebApiSafeLimitClass::Fallback *WebApiSafeLimitClass::getFallback(uint64_t inverterSerial) {
    for (auto &fallback : _fallback) {
        if (fallback._inverterSerial == inverterSerial) {
            return &fallback;
        }
    }
    for (auto &fallback : _fallback) {
        if (fallback._inverterSerial == 0) {
            fallback._inverterSerial = inverterSerial;
            fallback._currentLimit = 0;
            fallback._timeoutMillis = 0;
            return &fallback;
        }
    }
    return 0;
}

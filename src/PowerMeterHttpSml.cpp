// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterHttpSml.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <base64.h>
#include <ESPmDNS.h>

bool PowerMeterHttpSml::init()
{
    _upHttpGetter = std::make_unique<HttpGetter>(_cfg.HttpRequest);

    if (_upHttpGetter->init()) { return true; }

    MessageOutput.printf("[PowerMeterHttpSml] Initializing HTTP getter failed:\r\n");
    MessageOutput.printf("[PowerMeterHttpSml] %s\r\n", _upHttpGetter->getErrorText());

    _upHttpGetter = nullptr;

    return false;
}

void PowerMeterHttpSml::loop()
{
    if ((millis() - _lastPoll) < (_cfg.PollingInterval * 1000)) {
        return;
    }

    _lastPoll = millis();

    auto res = poll();
    if (!res.isEmpty()) {
        MessageOutput.printf("[PowerMeterHttpJson] %s\r\n", res.c_str());
        return;
    }

    gotUpdate();
}

bool PowerMeterHttpSml::isDataValid() const
{
    uint32_t age = millis() - getLastUpdate();
    return getLastUpdate() > 0 && (age < (3 * _cfg.PollingInterval * 1000));
}

String PowerMeterHttpSml::poll()
{
    if (!_upHttpGetter) {
        return "Initialization of HTTP request failed";
    }

    auto res = _upHttpGetter->performGetRequest();
    if (!res) {
        return _upHttpGetter->getErrorText();
    }

    auto pStream = res.getStream();
    if (!pStream) {
        return "Programmer error: HTTP request yields no stream";
    }

    while (pStream->available()) {
        processSmlByte(pStream->read());
    }

    return "";
}

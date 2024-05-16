// SPDX-License-Identifier: GPL-2.0-or-later
#include "Utils.h"
#include "Configuration.h"
#include "PowerMeterHttpJson.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "mbedtls/sha256.h"
#include <base64.h>
#include <ESPmDNS.h>

bool PowerMeterHttpJson::init()
{
    auto const& config = Configuration.get();

    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& valueConfig = config.PowerMeter.HttpJson[i];

        _httpGetters[i] = nullptr;

        if (i == 0 || (config.PowerMeter.HttpIndividualRequests && valueConfig.Enabled)) {
            _httpGetters[i] = std::make_unique<HttpGetter>(valueConfig.HttpRequest);
        }

        if (!_httpGetters[i]) { continue; }

        if (_httpGetters[i]->init()) {
            _httpGetters[i]->addHeader("Content-Type", "application/json");
            _httpGetters[i]->addHeader("Accept", "application/json");
            continue;
        }

        MessageOutput.printf("[PowerMeterHttpJson] Initializing HTTP getter for value %d failed:\r\n", i + 1);
        MessageOutput.printf("[PowerMeterHttpJson] %s\r\n", _httpGetters[i]->getErrorText());
        return false;
    }

    return true;
}

void PowerMeterHttpJson::loop()
{
    auto const& config = Configuration.get();
    if ((millis() - _lastPoll) < (config.PowerMeter.Interval * 1000)) {
        return;
    }

    _lastPoll = millis();

    auto res = poll();
    if (std::holds_alternative<String>(res)) {
        MessageOutput.printf("[PowerMeterHttpJson] %s\r\n", std::get<String>(res).c_str());
        return;
    }

    _powerValues = std::get<power_values_t>(res);
    gotUpdate();
}

PowerMeterHttpJson::poll_result_t PowerMeterHttpJson::poll()
{
    power_values_t cache;
    JsonDocument jsonResponse;

    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& cfg = Configuration.get().PowerMeter.HttpJson[i];

        if (!cfg.Enabled) {
            cache[i] = 0.0;
            continue;
        }

        auto const& upGetter = _httpGetters[i];

        if (upGetter) {
            auto res = upGetter->performGetRequest();
            if (!res) {
                return upGetter->getErrorText();
            }

            auto pStream = res.getStream();
            if (!pStream) {
                return String("Programmer error: HTTP request yields no stream");
            }

            const DeserializationError error = deserializeJson(jsonResponse, *pStream);
            if (error) {
                String msg("Unable to parse server response as JSON: ");
                return msg + error.c_str();
            }
        }

        auto pathResolutionResult = Utils::getJsonValueByPath<float>(jsonResponse, cfg.JsonPath);
        if (!pathResolutionResult.second.isEmpty()) {
            return pathResolutionResult.second;
        }

        // this value is supposed to be in Watts and positive if energy is consumed
        cache[i] = pathResolutionResult.first;

        switch (cfg.PowerUnit) {
            case Unit_t::MilliWatts:
                cache[i] /= 1000;
                break;
            case Unit_t::KiloWatts:
                cache[i] *= 1000;
                break;
            default:
                break;
        }

        if (cfg.SignInverted) { cache[i] *= -1; }
    }

    return cache;
}

float PowerMeterHttpJson::getPowerTotal() const
{
    float sum = 0.0;
    for (auto v: _powerValues) { sum += v; }
    return sum;
}

void PowerMeterHttpJson::doMqttPublish() const
{
    mqttPublish("power1", _powerValues[0]);
    mqttPublish("power2", _powerValues[1]);
    mqttPublish("power3", _powerValues[2]);
}

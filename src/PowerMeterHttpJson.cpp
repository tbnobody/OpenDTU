// SPDX-License-Identifier: GPL-2.0-or-later
#include "Utils.h"
#include "PowerMeterHttpJson.h"
#include "MessageOutput.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "mbedtls/sha256.h"
#include <base64.h>
#include <ESPmDNS.h>

PowerMeterHttpJson::~PowerMeterHttpJson()
{
    _taskDone = false;

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = true;
    lock.unlock();

    _cv.notify_all();

    if (_taskHandle != nullptr) {
        while (!_taskDone) { delay(10); }
        _taskHandle = nullptr;
    }
}

bool PowerMeterHttpJson::init()
{
    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& valueConfig = _cfg.Values[i];

        _httpGetters[i] = nullptr;

        if (i == 0 || (_cfg.IndividualRequests && valueConfig.Enabled)) {
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
    if (_taskHandle != nullptr) { return; }

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = false;
    lock.unlock();

    uint32_t constexpr stackSize = 3072;
    xTaskCreate(PowerMeterHttpJson::pollingLoopHelper, "PM:HTTP+JSON",
            stackSize, this, 1/*prio*/, &_taskHandle);
}

void PowerMeterHttpJson::pollingLoopHelper(void* context)
{
    auto pInstance = static_cast<PowerMeterHttpJson*>(context);
    pInstance->pollingLoop();
    pInstance->_taskDone = true;
    vTaskDelete(nullptr);
}

void PowerMeterHttpJson::pollingLoop()
{
    std::unique_lock<std::mutex> lock(_pollingMutex);

    while (!_stopPolling) {
        auto elapsedMillis = millis() - _lastPoll;
        auto intervalMillis = _cfg.PollingInterval * 1000;
        if (_lastPoll > 0 && elapsedMillis < intervalMillis) {
            auto sleepMs = intervalMillis - elapsedMillis;
            _cv.wait_for(lock, std::chrono::milliseconds(sleepMs),
                    [this] { return _stopPolling; }); // releases the mutex
            continue;
        }

        _lastPoll = millis();

        lock.unlock(); // polling can take quite some time
        auto res = poll();
        lock.lock();

        if (std::holds_alternative<String>(res)) {
            MessageOutput.printf("[PowerMeterHttpJson] %s\r\n", std::get<String>(res).c_str());
            continue;
        }

        MessageOutput.printf("[PowerMeterHttpJson] New total: %.2f\r\n", getPowerTotal());

        gotUpdate();
    }
}

PowerMeterHttpJson::poll_result_t PowerMeterHttpJson::poll()
{
    power_values_t cache;
    JsonDocument jsonResponse;

    auto prefixedError = [](uint8_t idx, char const* err) -> String {
        String res("Value ");
        res.reserve(strlen(err) + 16);
        return res + String(idx + 1) + ": " + err;
    };

    for (uint8_t i = 0; i < POWERMETER_HTTP_JSON_MAX_VALUES; i++) {
        auto const& cfg = _cfg.Values[i];

        if (!cfg.Enabled) {
            cache[i] = 0.0;
            continue;
        }

        auto const& upGetter = _httpGetters[i];

        if (upGetter) {
            auto res = upGetter->performGetRequest();
            if (!res) {
                return prefixedError(i, upGetter->getErrorText());
            }

            auto pStream = res.getStream();
            if (!pStream) {
                return prefixedError(i, "Programmer error: HTTP request yields no stream");
            }

            const DeserializationError error = deserializeJson(jsonResponse, *pStream);
            if (error) {
                String msg("Unable to parse server response as JSON: ");
                return prefixedError(i, String(msg + error.c_str()).c_str());
            }
        }

        auto pathResolutionResult = Utils::getJsonValueByPath<float>(jsonResponse, cfg.JsonPath);
        if (!pathResolutionResult.second.isEmpty()) {
            return prefixedError(i, pathResolutionResult.second.c_str());
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

    std::unique_lock<std::mutex> lock(_valueMutex);
    _powerValues = cache;
    return cache;
}

float PowerMeterHttpJson::getPowerTotal() const
{
    float sum = 0.0;
    std::unique_lock<std::mutex> lock(_valueMutex);
    for (auto v: _powerValues) { sum += v; }
    return sum;
}

bool PowerMeterHttpJson::isDataValid() const
{
    uint32_t age = millis() - getLastUpdate();
    return getLastUpdate() > 0 && (age < (3 * _cfg.PollingInterval * 1000));
}

void PowerMeterHttpJson::doMqttPublish() const
{
    std::unique_lock<std::mutex> lock(_valueMutex);
    mqttPublish("power1", _powerValues[0]);
    mqttPublish("power2", _powerValues[1]);
    mqttPublish("power3", _powerValues[2]);
}

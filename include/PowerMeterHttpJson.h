// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <array>
#include <variant>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <stdint.h>
#include "HttpGetter.h"
#include "Configuration.h"
#include "PowerMeterProvider.h"

using Auth_t = HttpRequestConfig::Auth;
using Unit_t = PowerMeterHttpJsonValue::Unit;

class PowerMeterHttpJson : public PowerMeterProvider {
public:
    explicit PowerMeterHttpJson(PowerMeterHttpJsonConfig const& cfg)
        : _cfg(cfg) { }

    ~PowerMeterHttpJson();

    bool init() final;
    void loop() final;
    float getPowerTotal() const final;
    bool isDataValid() const final;
    void doMqttPublish() const final;

    using power_values_t = std::array<float, POWERMETER_HTTP_JSON_MAX_VALUES>;
    using poll_result_t = std::variant<power_values_t, String>;
    poll_result_t poll();

private:
    static void pollingLoopHelper(void* context);
    std::atomic<bool> _taskDone;
    void pollingLoop();

    PowerMeterHttpJsonConfig const _cfg;

    uint32_t _lastPoll = 0;

    mutable std::mutex _valueMutex;
    power_values_t _powerValues = {};

    std::array<std::unique_ptr<HttpGetter>, POWERMETER_HTTP_JSON_MAX_VALUES> _httpGetters;

    TaskHandle_t _taskHandle = nullptr;
    bool _stopPolling;
    mutable std::mutex _pollingMutex;
    std::condition_variable _cv;
};

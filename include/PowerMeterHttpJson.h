// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <variant>
#include <memory>
#include <stdint.h>
#include "HttpGetter.h"
#include "Configuration.h"
#include "PowerMeterProvider.h"

using Auth_t = HttpRequestConfig::Auth;
using Unit_t = PowerMeterHttpJsonValue::Unit;

class PowerMeterHttpJson : public PowerMeterProvider {
public:
    bool init() final;
    void loop() final;
    float getPowerTotal() const final;
    void doMqttPublish() const final;

    using power_values_t = std::array<float, POWERMETER_HTTP_JSON_MAX_VALUES>;
    using poll_result_t = std::variant<power_values_t, String>;
    poll_result_t poll();

private:
    uint32_t _lastPoll;

    power_values_t _powerValues;

    std::array<std::unique_ptr<HttpGetter>, POWERMETER_HTTP_JSON_MAX_VALUES> _httpGetters;
};

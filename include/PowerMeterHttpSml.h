// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <stdint.h>
#include <Arduino.h>
#include "HttpGetter.h"
#include "Configuration.h"
#include "PowerMeterSml.h"

class PowerMeterHttpSml : public PowerMeterSml {
public:
    explicit PowerMeterHttpSml(PowerMeterHttpSmlConfig const& cfg)
        : _cfg(cfg) { }

    bool init() final;
    void loop() final;
    bool isDataValid() const final;

    // returns an empty string on success,
    // returns an error message otherwise.
    String poll();

private:
    PowerMeterHttpSmlConfig const _cfg;

    uint32_t _lastPoll = 0;

    std::unique_ptr<HttpGetter> _upHttpGetter;
};

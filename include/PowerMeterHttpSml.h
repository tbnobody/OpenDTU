// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <stdint.h>
#include <Arduino.h>
#include "HttpGetter.h"
#include "PowerMeterSml.h"

class PowerMeterHttpSml : public PowerMeterSml {
public:
    bool init() final;
    void loop() final;

    // returns an empty string on success,
    // returns an error message otherwise.
    String poll();

private:
    uint32_t _lastPoll = 0;

    std::unique_ptr<HttpGetter> _upHttpGetter;
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterSml.h"
#include <SoftwareSerial.h>

class PowerMeterSerialSml : public PowerMeterSml {
public:
    bool init() final;
    void deinit() final;
    void loop() final;

private:
    std::unique_ptr<SoftwareSerial> _upSmlSerial = nullptr;
};

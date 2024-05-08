// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#pragma once

#include <cstdint>
#include "PowerMeterProvider.h"

class PowerMeterUdpSmaHomeManager : public PowerMeterProvider {
public:
    bool init() final;
    void deinit() final;
    void loop() final;
    float getPowerTotal() const final { return _powerMeterPower; }
    void doMqttPublish() const final;

private:
    void Soutput(int kanal, int index, int art, int tarif,
            char const* name, float value, uint32_t timestamp);

    uint8_t* decodeGroup(uint8_t* offset, uint16_t grouplen);

    float _powerMeterPower = 0.0;
    float _powerMeterL1 = 0.0;
    float _powerMeterL2 = 0.0;
    float _powerMeterL3 = 0.0;
    uint32_t _previousMillis = 0;
    uint32_t _serial = 0;
};

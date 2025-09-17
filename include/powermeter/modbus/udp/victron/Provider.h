// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#pragma once

#include <cstdint>
#include <Configuration.h>
#include <powermeter/Provider.h>

namespace PowerMeters::Modbus::Udp::Victron {

class Provider : public ::PowerMeters::Provider {
public:
    explicit Provider(PowerMeterUdpVictronConfig const& cfg);
    ~Provider();

    bool init() final;
    void loop() final;

private:
    void sendModbusRequest();
    void parseModbusResponse();

    uint32_t _lastRequest = 0;
    PowerMeterUdpVictronConfig _cfg;
};

} // namespace PowerMeters::Modbus::Udp::Victron

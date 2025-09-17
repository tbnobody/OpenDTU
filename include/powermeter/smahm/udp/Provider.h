// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#pragma once

#include <cstdint>
#include <powermeter/Provider.h>

namespace PowerMeters::SmaHM::Udp {

class Provider : public ::PowerMeters::Provider {
public:
    ~Provider();

    bool init() final;
    void loop() final;

private:
    void Soutput(int kanal, int index, int art, int tarif,
            char const* name, float value, uint32_t timestamp);

    uint8_t* decodeGroup(uint8_t* offset, uint16_t grouplen);

    uint32_t _previousMillis = 0;
    uint32_t _serial = 0;
};

} // namespace PowerMeters::SmaHM::Udp

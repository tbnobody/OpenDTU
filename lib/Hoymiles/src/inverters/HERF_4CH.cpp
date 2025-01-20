// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "HERF_4CH.h"

HERF_4CH::HERF_4CH(HoymilesRadio* radio, const uint64_t serial)
    : HM_4CH(radio, serial)
{
}

bool HERF_4CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x280100000000 && serial <= 0x2801ffffffff
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x2801;
}

String HERF_4CH::typeName() const
{
    return "HERF-1600/1800-4T";
}

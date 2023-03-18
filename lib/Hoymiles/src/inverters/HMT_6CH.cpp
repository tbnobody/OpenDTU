// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMT_6CH.h"

HMT_6CH::HMT_6CH(HoymilesRadio* radio, uint64_t serial)
    : HMT_Abstract(radio, serial) {};

bool HMT_6CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x138200000000 && serial <= 0x138299999999
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1382;
}

String HMT_6CH::typeName()
{
    return F("HMT-1800, HMT-2250");
}

const std::list<byteAssign_t>* HMT_6CH::getByteAssignment()
{
    return &byteAssignment;
}
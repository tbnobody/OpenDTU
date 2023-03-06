// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMS_4CH.h"

HMS_4CH::HMS_4CH(HoymilesRadio* radio, uint64_t serial)
    : HMS_Abstract(radio, serial) {};

bool HMS_4CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x114400000000 && serial <= 0x114499999999
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1164;
}

String HMS_4CH::typeName()
{
    return "HMS-1600, HMS-1800, HMS-2000";
}

const std::list<byteAssign_t>* HMS_4CH::getByteAssignment()
{
    return &byteAssignment;
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMS_2CH.h"

HMS_2CH::HMS_2CH(HoymilesRadio* radio, uint64_t serial)
    : HMS_Abstract(radio, serial) {};

bool HMS_2CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x114400000000 && serial <= 0x114499999999
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1144;
}

String HMS_2CH::typeName()
{
    return "HMS-600, HMS-700, HMS-800, HMS-900, HMS-1000";
}

const std::list<byteAssign_t>* HMS_2CH::getByteAssignment()
{
    return &byteAssignment;
}
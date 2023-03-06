// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMS_1CH.h"

HMS_1CH::HMS_1CH(HoymilesRadio* radio, uint64_t serial)
    : HMS_Abstract(radio, serial) {};

bool HMS_1CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x112400000000 && serial <= 0x112499999999
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1124;
}

String HMS_1CH::typeName()
{
    return "HMS-300, HMS-350, HMS-400, HMS-450, HMS-500";
}

const std::list<byteAssign_t>* HMS_1CH::getByteAssignment()
{
    return &byteAssignment;
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "HM_2CH.h"

HM_2CH::HM_2CH(HoymilesRadio* radio, uint64_t serial)
    : HM_Abstract(radio, serial) {};

bool HM_2CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x114100000000 && serial <= 0x114199999999

    uint8_t preId[2];
    preId[0] = (uint8_t)(serial >> 40);
    preId[1] = (uint8_t)(serial >> 32);

    if ((uint8_t)(((((uint16_t)preId[0] << 8) | preId[1]) >> 4) & 0xff) == 0x14) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x30) || ((preId[1] & 0xf0) == 0x40))
        && (((preId[0] == 0x10) && (preId[1] == 0x42)) || ((preId[0] == 0x11) && (preId[1] == 0x41)))) {
        return true;
    }

    return false;
}

String HM_2CH::typeName()
{
    return "HM-600, HM-700, HM-800";
}

const std::list<byteAssign_t>* HM_2CH::getByteAssignment()
{
    return &byteAssignment;
}
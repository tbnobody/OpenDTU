// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "HM_4CH.h"

HM_4CH::HM_4CH(HoymilesRadio* radio, uint64_t serial)
    : HM_Abstract(radio, serial) {};

bool HM_4CH::isValidSerial(uint64_t serial)
{
    // serial >= 0x116100000000 && serial <= 0x116199999999

    uint8_t preId[2];
    preId[0] = (uint8_t)(serial >> 40);
    preId[1] = (uint8_t)(serial >> 32);

    if ((uint8_t)(((((uint16_t)preId[0] << 8) | preId[1]) >> 4) & 0xff) == 0x16) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x50) || ((preId[1] & 0xf0) == 0x60))
        && (((preId[0] == 0x10) && (preId[1] == 0x62)) || ((preId[0] == 0x11) && (preId[1] == 0x61)))) {
        return true;
    }

    return false;
}

String HM_4CH::typeName()
{
    return "HM-1000, HM-1200, HM-1500";
}

const std::list<byteAssign_t>* HM_4CH::getByteAssignment()
{
    return &byteAssignment;
}
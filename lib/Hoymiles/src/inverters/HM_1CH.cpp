// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "HM_1CH.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 6, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 12, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 8, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 14, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 22, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 18, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 20, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 16, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 24, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 26, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 28, 2, 1, false, 0 },

    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

HM_1CH::HM_1CH(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
}

bool HM_1CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x112100000000 && serial <= 0x1121ffffffff

    uint8_t preId[2];
    preId[0] = static_cast<uint8_t>(serial >> 40);
    preId[1] = static_cast<uint8_t>(serial >> 32);

    if (static_cast<uint8_t>((((static_cast<uint16_t>(preId[0]) << 8) | preId[1]) >> 4) & 0xff) == 0x12) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x10) || ((preId[1] & 0xf0) == 0x20))
        && (((preId[0] == 0x10) && (preId[1] == 0x22)) || ((preId[0] == 0x11) && (preId[1] == 0x21)))) {
        return true;
    }

    return false;
}

String HM_1CH::typeName() const
{
    return "HM-300/350/400-1T";
}

const byteAssign_t* HM_1CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t HM_1CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "HM_4CH.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 8, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 20, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 12, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    { TYPE_DC, CH1, FLD_UDC, UNIT_V, CALC_CH_UDC, CH0, CMD_CALC, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A, 6, 2, 100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W, 10, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
    { TYPE_DC, CH1, FLD_YT, UNIT_KWH, 16, 4, 1000, false, 3 },
    { TYPE_DC, CH1, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH1, CMD_CALC, false, 3 },

    { TYPE_DC, CH2, FLD_UDC, UNIT_V, 24, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_IDC, UNIT_A, 26, 2, 100, false, 2 },
    { TYPE_DC, CH2, FLD_PDC, UNIT_W, 30, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_YD, UNIT_WH, 42, 2, 1, false, 0 },
    { TYPE_DC, CH2, FLD_YT, UNIT_KWH, 34, 4, 1000, false, 3 },
    { TYPE_DC, CH2, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH2, CMD_CALC, false, 3 },

    { TYPE_DC, CH3, FLD_UDC, UNIT_V, CALC_CH_UDC, CH2, CMD_CALC, false, 1 },
    { TYPE_DC, CH3, FLD_IDC, UNIT_A, 28, 2, 100, false, 2 },
    { TYPE_DC, CH3, FLD_PDC, UNIT_W, 32, 2, 10, false, 1 },
    { TYPE_DC, CH3, FLD_YD, UNIT_WH, 44, 2, 1, false, 0 },
    { TYPE_DC, CH3, FLD_YT, UNIT_KWH, 38, 4, 1000, false, 3 },
    { TYPE_DC, CH3, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH3, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 46, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 54, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 50, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 52, 2, 10, true, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 48, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 56, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 58, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 60, 2, 1, false, 0 },

    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

HM_4CH::HM_4CH(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
}

bool HM_4CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x116100000000 && serial <= 0x1161ffffffff

    uint8_t preId[2];
    preId[0] = static_cast<uint8_t>(serial >> 40);
    preId[1] = static_cast<uint8_t>(serial >> 32);

    if (static_cast<uint8_t>((((static_cast<uint16_t>(preId[0]) << 8) | preId[1]) >> 4) & 0xff) == 0x16) {
        return true;
    }

    if ((((preId[1] & 0xf0) == 0x50) || ((preId[1] & 0xf0) == 0x60))
        && (((preId[0] == 0x10) && (preId[1] == 0x62)) || ((preId[0] == 0x11) && (preId[1] == 0x61)))) {
        return true;
    }

    return false;
}

String HM_4CH::typeName() const
{
    return "HM-1000/1200/1500-4T";
}

const byteAssign_t* HM_4CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t HM_4CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

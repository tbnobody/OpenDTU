// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HMS_1CHv2.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 6, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 10, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 14, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH0, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 26, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 34, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 30, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 20, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 28, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 36, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 38, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 18, 2, 1, false, 0 },

    { TYPE_AC, CH0, FLD_YD, UNIT_WH, CALC_YD_CH0, 0, CMD_CALC, false, 0 },
    { TYPE_AC, CH0, FLD_YT, UNIT_KWH, CALC_YT_CH0, 0, CMD_CALC, false, 3 },
    { TYPE_AC, CH0, FLD_PDC, UNIT_W, CALC_PDC_CH0, 0, CMD_CALC, false, 1 },
    { TYPE_AC, CH0, FLD_EFF, UNIT_PCT, CALC_EFF_CH0, 0, CMD_CALC, false, 3 }
};

HMS_1CHv2::HMS_1CHv2(HoymilesRadio* radio, uint64_t serial)
    : HMS_Abstract(radio, serial) {};

bool HMS_1CHv2::isValidSerial(uint64_t serial)
{
    // serial >= 0x112500000000 && serial <= 0x112599999999
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1125;
}

String HMS_1CHv2::typeName()
{
    return "HMS-500 v2";
}

const byteAssign_t* HMS_1CHv2::getByteAssignment()
{
    return byteAssignment;
}

uint8_t HMS_1CHv2::getByteAssignmentSize()
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}
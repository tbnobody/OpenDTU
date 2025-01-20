// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "HMS_4CH.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 6, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 10, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 14, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    { TYPE_DC, CH1, FLD_UDC, UNIT_V, 4, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A, 8, 2, 100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W, 12, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_YD, UNIT_WH, 24, 2, 1, false, 0 },
    { TYPE_DC, CH1, FLD_YT, UNIT_KWH, 18, 4, 1000, false, 3 },
    { TYPE_DC, CH1, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH1, CMD_CALC, false, 3 },

    { TYPE_DC, CH2, FLD_UDC, UNIT_V, 26, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_IDC, UNIT_A, 30, 2, 100, false, 2 },
    { TYPE_DC, CH2, FLD_PDC, UNIT_W, 34, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_YD, UNIT_WH, 46, 2, 1, false, 0 },
    { TYPE_DC, CH2, FLD_YT, UNIT_KWH, 38, 4, 1000, false, 3 },
    { TYPE_DC, CH2, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH2, CMD_CALC, false, 3 },

    { TYPE_DC, CH3, FLD_UDC, UNIT_V, 28, 2, 10, false, 1 },
    { TYPE_DC, CH3, FLD_IDC, UNIT_A, 32, 2, 100, false, 2 },
    { TYPE_DC, CH3, FLD_PDC, UNIT_W, 36, 2, 10, false, 1 },
    { TYPE_DC, CH3, FLD_YD, UNIT_WH, 48, 2, 1, false, 0 },
    { TYPE_DC, CH3, FLD_YT, UNIT_KWH, 42, 4, 1000, false, 3 },
    { TYPE_DC, CH3, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH3, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 50, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 58, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 54, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 56, 2, 10, true, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 52, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 60, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 62, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 64, 2, 1, false, 0 },

    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

HMS_4CH::HMS_4CH(HoymilesRadio* radio, const uint64_t serial)
    : HMS_Abstract(radio, serial)
{
}

bool HMS_4CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x116400000000 && serial <= 0x1164ffffffff
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1164;
}

String HMS_4CH::typeName() const
{
    return "HMS-1600/1800/2000-4T";
}

const byteAssign_t* HMS_4CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t HMS_4CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

bool HMS_4CH::supportsPowerDistributionLogic()
{
    // This feature was added in inverter firmware version 01.01.12 and
    // will limit the AC output instead of limiting the DC inputs.
    return DevInfo()->getFwBuildVersion() >= 10112U;
}

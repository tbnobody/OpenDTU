
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "HERF_1CH.h"

static const byteAssign_t byteAssignment[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 6, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 10, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 14, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 26, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, 34, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 30, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 40, 2, 10, false, 1 }, // to be verified
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 28, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 36, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T, UNIT_C, 38, 2, 10, true, 1 }, // to be verified
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 40, 2, 1, false, 0 }, // to be verified

    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

HERF_1CH::HERF_1CH(HoymilesRadio* radio, const uint64_t serial)
    : HM_Abstract(radio, serial)
{
}

bool HERF_1CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x284100000000 && serial <= 0x2841ffffffff
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x2841;
}

String HERF_1CH::typeName() const
{
    return "HERF-300-1T";
}

const byteAssign_t* HERF_1CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t HERF_1CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

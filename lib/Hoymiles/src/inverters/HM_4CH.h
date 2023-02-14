// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HM_4CH : public HM_Abstract {
public:
    explicit HM_4CH(uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    const std::list<byteAssign_t>* getByteAssignment();

private:
    const std::list<byteAssign_t> byteAssignment = {
        { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
        { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
        { TYPE_DC, CH0, FLD_PDC, UNIT_W, 8, 2, 10, false, 1 },
        { TYPE_DC, CH0, FLD_YD, UNIT_WH, 20, 2, 1, false, 0 },
        { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 12, 4, 1000, false, 3 },
        { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH0, CMD_CALC, false, 3 },

        { TYPE_DC, CH1, FLD_UDC, UNIT_V, CALC_UDC_CH, CH0, CMD_CALC, false, 1 },
        { TYPE_DC, CH1, FLD_IDC, UNIT_A, 6, 2, 100, false, 2 },
        { TYPE_DC, CH1, FLD_PDC, UNIT_W, 10, 2, 10, false, 1 },
        { TYPE_DC, CH1, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
        { TYPE_DC, CH1, FLD_YT, UNIT_KWH, 16, 4, 1000, false, 3 },
        { TYPE_DC, CH1, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH1, CMD_CALC, false, 3 },

        { TYPE_DC, CH2, FLD_UDC, UNIT_V, 24, 2, 10, false, 1 },
        { TYPE_DC, CH2, FLD_IDC, UNIT_A, 26, 2, 100, false, 2 },
        { TYPE_DC, CH2, FLD_PDC, UNIT_W, 30, 2, 10, false, 1 },
        { TYPE_DC, CH2, FLD_YD, UNIT_WH, 42, 2, 1, false, 0 },
        { TYPE_DC, CH2, FLD_YT, UNIT_KWH, 34, 4, 1000, false, 3 },
        { TYPE_DC, CH2, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH2, CMD_CALC, false, 3 },

        { TYPE_DC, CH3, FLD_UDC, UNIT_V, CALC_UDC_CH, CH2, CMD_CALC, false, 1 },
        { TYPE_DC, CH3, FLD_IDC, UNIT_A, 28, 2, 100, false, 2 },
        { TYPE_DC, CH3, FLD_PDC, UNIT_W, 32, 2, 10, false, 1 },
        { TYPE_DC, CH3, FLD_YD, UNIT_WH, 44, 2, 1, false, 0 },
        { TYPE_DC, CH3, FLD_YT, UNIT_KWH, 38, 4, 1000, false, 3 },
        { TYPE_DC, CH3, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH3, CMD_CALC, false, 3 },

        { TYPE_AC, CH0, FLD_UAC, UNIT_V, 46, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_IAC, UNIT_A, 54, 2, 100, false, 2 },
        { TYPE_AC, CH0, FLD_PAC, UNIT_W, 50, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_PRA, UNIT_VA, 52, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_F, UNIT_HZ, 48, 2, 100, false, 2 },
        { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 56, 2, 1000, false, 3 },

        { TYPE_INV, CH0, FLD_T, UNIT_C, 58, 2, 10, true, 1 },
        { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 60, 2, 1, false, 0 },

        { TYPE_AC, CH0, FLD_YD, UNIT_WH, CALC_YD_CH0, 0, CMD_CALC, false, 0 },
        { TYPE_AC, CH0, FLD_YT, UNIT_KWH, CALC_YT_CH0, 0, CMD_CALC, false, 3 },
        { TYPE_AC, CH0, FLD_PDC, UNIT_W, CALC_PDC_CH0, 0, CMD_CALC, false, 1 },
        { TYPE_AC, CH0, FLD_EFF, UNIT_PCT, CALC_EFF_CH0, 0, CMD_CALC, false, 3 }
    };
};
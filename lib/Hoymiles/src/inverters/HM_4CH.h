// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HM_4CH : public HM_Abstract {
public:
    explicit HM_4CH(uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    const byteAssign_t* getByteAssignment();
    uint8_t getAssignmentCount();

private:
    const byteAssign_t byteAssignment[36] = {
        { FLD_UDC, UNIT_V, CH1, 2, 2, 10, false },
        { FLD_IDC, UNIT_A, CH1, 4, 2, 100, false },
        { FLD_PDC, UNIT_W, CH1, 8, 2, 10, false },
        { FLD_YD, UNIT_WH, CH1, 20, 2, 1, false },
        { FLD_YT, UNIT_KWH, CH1, 12, 4, 1000, false },
        { FLD_IRR, UNIT_PCT, CH1, CALC_IRR_CH, CH1, CMD_CALC, false },

        { FLD_UDC, UNIT_V, CH2, CALC_UDC_CH, CH1, CMD_CALC, false },
        { FLD_IDC, UNIT_A, CH2, 6, 2, 100, false },
        { FLD_PDC, UNIT_W, CH2, 10, 2, 10, false },
        { FLD_YD, UNIT_WH, CH2, 22, 2, 1, false },
        { FLD_YT, UNIT_KWH, CH2, 16, 4, 1000, false },
        { FLD_IRR, UNIT_PCT, CH2, CALC_IRR_CH, CH2, CMD_CALC, false },

        { FLD_UDC, UNIT_V, CH3, 24, 2, 10, false },
        { FLD_IDC, UNIT_A, CH3, 26, 2, 100, false },
        { FLD_PDC, UNIT_W, CH3, 30, 2, 10, false },
        { FLD_YD, UNIT_WH, CH3, 42, 2, 1, false },
        { FLD_YT, UNIT_KWH, CH3, 34, 4, 1000, false },
        { FLD_IRR, UNIT_PCT, CH3, CALC_IRR_CH, CH3, CMD_CALC, false },

        { FLD_UDC, UNIT_V, CH4, CALC_UDC_CH, CH3, CMD_CALC, false },
        { FLD_IDC, UNIT_A, CH4, 28, 2, 100, false },
        { FLD_PDC, UNIT_W, CH4, 32, 2, 10, false },
        { FLD_YD, UNIT_WH, CH4, 44, 2, 1, false },
        { FLD_YT, UNIT_KWH, CH4, 38, 4, 1000, false },
        { FLD_IRR, UNIT_PCT, CH4, CALC_IRR_CH, CH4, CMD_CALC, false },

        { FLD_UAC, UNIT_V, CH0, 46, 2, 10, false },
        { FLD_IAC, UNIT_A, CH0, 54, 2, 100, false },
        { FLD_PAC, UNIT_W, CH0, 50, 2, 10, false },
        { FLD_PRA, UNIT_VA, CH0, 52, 2, 10, false },
        { FLD_F, UNIT_HZ, CH0, 48, 2, 100, false },
        { FLD_PF, UNIT_NONE, CH0, 56, 2, 1000, false },
        { FLD_T, UNIT_C, CH0, 58, 2, 10, true },
        { FLD_EVT_LOG, UNIT_NONE, CH0, 60, 2, 1, false },
        { FLD_YD, UNIT_WH, CH0, CALC_YD_CH0, 0, CMD_CALC, false },
        { FLD_YT, UNIT_KWH, CH0, CALC_YT_CH0, 0, CMD_CALC, false },
        { FLD_PDC, UNIT_W, CH0, CALC_PDC_CH0, 0, CMD_CALC, false },
        { FLD_EFF, UNIT_PCT, CH0, CALC_EFF_CH0, 0, CMD_CALC, false }
    };
};
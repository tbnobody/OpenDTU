// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HM_Abstract.h"

class HM_2CH : public HM_Abstract {
public:
    explicit HM_2CH(uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    const byteAssign_t* getByteAssignment();
    uint8_t getAssignmentCount();

private:
    const byteAssign_t byteAssignment[24] = {
        { CH1, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
        { CH1, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
        { CH1, FLD_PDC, UNIT_W, 6, 2, 10, false, 1 },
        { CH1, FLD_YD, UNIT_WH, 22, 2, 1, false, 0 },
        { CH1, FLD_YT, UNIT_KWH, 14, 4, 1000, false, 3 },
        { CH1, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH1, CMD_CALC, false, 3 },

        { CH2, FLD_UDC, UNIT_V, 8, 2, 10, false, 1 },
        { CH2, FLD_IDC, UNIT_A, 10, 2, 100, false, 2 },
        { CH2, FLD_PDC, UNIT_W, 12, 2, 10, false, 1 },
        { CH2, FLD_YD, UNIT_WH, 24, 2, 1, false, 0 },
        { CH2, FLD_YT, UNIT_KWH, 18, 4, 1000, false, 3 },
        { CH2, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH2, CMD_CALC, false, 3 },

        { CH0, FLD_UAC, UNIT_V, 26, 2, 10, false, 1 },
        { CH0, FLD_IAC, UNIT_A, 34, 2, 100, false, 2 },
        { CH0, FLD_PAC, UNIT_W, 30, 2, 10, false, 1 },
        { CH0, FLD_PRA, UNIT_VA, 32, 2, 10, false, 1 },
        { CH0, FLD_F, UNIT_HZ, 28, 2, 100, false, 2 },
        { CH0, FLD_PF, UNIT_NONE, 36, 2, 1000, false, 3 },
        { CH0, FLD_T, UNIT_C, 38, 2, 10, true, 1 },
        { CH0, FLD_EVT_LOG, UNIT_NONE, 40, 2, 1, false, 0 },
        { CH0, FLD_YD, UNIT_WH, CALC_YD_CH0, 0, CMD_CALC, false, 0 },
        { CH0, FLD_YT, UNIT_KWH, CALC_YT_CH0, 0, CMD_CALC, false, 3 },
        { CH0, FLD_PDC, UNIT_W, CALC_PDC_CH0, 0, CMD_CALC, false, 1 },
        { CH0, FLD_EFF, UNIT_PCT, CALC_EFF_CH0, 0, CMD_CALC, false, 3 }
    };
};
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HMS_Abstract.h"
#include <list>

class HMS_1CH : public HMS_Abstract {
public:
    explicit HMS_1CH(HoymilesRadio* radio, uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName();
    const std::list<byteAssign_t>* getByteAssignment();

private:
    const std::list<byteAssign_t> byteAssignment = {
        { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
        { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
        { TYPE_DC, CH0, FLD_PDC, UNIT_W, 6, 2, 10, false, 1 },
        { TYPE_DC, CH0, FLD_YD, UNIT_WH, 12, 2, 1, false, 0 },
        { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 8, 4, 1000, false, 3 },
        { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_IRR_CH, CH0, CMD_CALC, false, 3 },

        { TYPE_AC, CH0, FLD_UAC, UNIT_V, 14, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_IAC, UNIT_A, 22, 2, 100, false, 2 },
        { TYPE_AC, CH0, FLD_PAC, UNIT_W, 18, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_PRA, UNIT_VA, 20, 2, 10, false, 1 },
        { TYPE_AC, CH0, FLD_F, UNIT_HZ, 16, 2, 100, false, 2 },
        { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 24, 2, 1000, false, 3 },

        { TYPE_INV, CH0, FLD_T, UNIT_C, 26, 2, 10, true, 1 },
        { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 28, 2, 1, false, 0 },

        { TYPE_AC, CH0, FLD_YD, UNIT_WH, CALC_YD_CH0, 0, CMD_CALC, false, 0 },
        { TYPE_AC, CH0, FLD_YT, UNIT_KWH, CALC_YT_CH0, 0, CMD_CALC, false, 3 },
        { TYPE_AC, CH0, FLD_PDC, UNIT_W, CALC_PDC_CH0, 0, CMD_CALC, false, 1 },
        { TYPE_AC, CH0, FLD_EFF, UNIT_PCT, CALC_EFF_CH0, 0, CMD_CALC, false, 3 }
    };
};
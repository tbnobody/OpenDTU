#pragma once

#include "HM_Abstract.h"

class HM_2CH : public HM_Abstract {
public:
    HM_2CH(uint64_t serial);
    static bool isValidSerial(uint64_t serial);
    String typeName() override;
    const byteAssign_t* getByteAssignment() override;
    const uint8_t getAssignmentCount() override;

private:
    const byteAssign_t byteAssignment[23] = {
        { FLD_UDC, UNIT_V, CH1, 2, 2, 10 },
        { FLD_IDC, UNIT_A, CH1, 4, 2, 100 },
        { FLD_PDC, UNIT_W, CH1, 6, 2, 10 },
        { FLD_YD, UNIT_WH, CH1, 22, 2, 1 },
        { FLD_YT, UNIT_KWH, CH1, 14, 4, 1000 },
        { FLD_IRR, UNIT_PCT, CH1, CALC_IRR_CH, CH1, CMD_CALC },

        { FLD_UDC, UNIT_V, CH2, 8, 2, 10 },
        { FLD_IDC, UNIT_A, CH2, 10, 2, 100 },
        { FLD_PDC, UNIT_W, CH2, 12, 2, 10 },
        { FLD_YD, UNIT_WH, CH2, 24, 2, 1 },
        { FLD_YT, UNIT_KWH, CH2, 18, 4, 1000 },
        { FLD_IRR, UNIT_PCT, CH2, CALC_IRR_CH, CH2, CMD_CALC },

        { FLD_UAC, UNIT_V, CH0, 26, 2, 10 },
        { FLD_IAC, UNIT_A, CH0, 34, 2, 100 },
        { FLD_PAC, UNIT_W, CH0, 30, 2, 10 },
        { FLD_PRA, UNIT_VA, CH0, 32, 2, 10 },
        { FLD_F, UNIT_HZ, CH0, 28, 2, 100 },
        { FLD_T, UNIT_C, CH0, 38, 2, 10 },
        { FLD_EVT_LOG, UNIT_CNT, CH0, 40, 2, 1 },
        { FLD_YD, UNIT_WH, CH0, CALC_YD_CH0, 0, CMD_CALC },
        { FLD_YT, UNIT_KWH, CH0, CALC_YT_CH0, 0, CMD_CALC },
        { FLD_PDC, UNIT_W, CH0, CALC_PDC_CH0, 0, CMD_CALC },
        { FLD_EFF, UNIT_PCT, CH0, CALC_EFF_CH0, 0, CMD_CALC }
    };
};
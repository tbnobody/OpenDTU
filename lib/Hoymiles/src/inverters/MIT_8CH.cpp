// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 - OpenDTU Contributors
 *
 * MIT-5000-8T support (3-phase, 4 MPPT inputs)
 *
 * Byte assignments validated against RF captures from a real MIT-5000-8T
 * (serial prefix 0x1520, SN 1520a025566b) cross-referenced with DTU-Pro
 * Modbus TCP data via Home Assistant sensors.
 *
 * Despite the "8T" name, the inverter reports 4 independent MPPT channels
 * over RF (each MPPT tracker has 2 panel inputs sharing a single tracker).
 *
 * Payload structure: 92 bytes across 6 RF frames (01-05 + 0x86)
 *   - Each frame: 27 bytes total = 9 header + 1 frameID + 16 data + 1 CRC8
 *   - Frames 01-05: 16 data bytes each = 80 bytes
 *   - Frame 0x86: 12 data bytes (shorter, includes 2-byte CRC16 over full payload)
 *   - Reassembled payload: 90 usable bytes (88 data + 2 CRC16)
 *
 * RF capture validation (2026-02-01, ~230W production):
 *   MPPT1: UDC=33.7V IDC=1.03A PDC=34.9W YT=80.368kWh
 *   MPPT2: UDC=35.4V IDC=1.02A PDC=36.8W YT=65.066kWh
 *   MPPT3: UDC=72.3V IDC=1.26A PDC=91.3W YT=155.147kWh
 *   MPPT4: UDC=69.3V IDC=1.11A PDC=77.1W YT=151.566kWh
 *   AC: UAC_1N=231.7V F=50.00Hz PAC=225.4W Q=0VAR PF=1.000
 *   INV: T=12.0°C
 *
 * Unresolved offsets:
 *   68: always 0 in captures so far (reserved?)
 *   70: value matches PAC — possibly apparent power (S in VA)
 */
#include "MIT_8CH.h"

static const byteAssign_t byteAssignment[] = {
    // DC Channel 0 (MPPT1) - offset 2
    { TYPE_DC, CH0, FLD_UDC, UNIT_V, 2, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A, 4, 2, 100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W, 6, 2, 10, false, 1 },
    { TYPE_DC, CH0, FLD_YT, UNIT_KWH, 8, 4, 1000, false, 3 },
    { TYPE_DC, CH0, FLD_YD, UNIT_WH, 12, 2, 1, false, 0 },
    { TYPE_DC, CH0, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH0, CMD_CALC, false, 3 },

    // DC Channel 1 (MPPT2) - offset 14
    { TYPE_DC, CH1, FLD_UDC, UNIT_V, 14, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A, 16, 2, 100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W, 18, 2, 10, false, 1 },
    { TYPE_DC, CH1, FLD_YT, UNIT_KWH, 20, 4, 1000, false, 3 },
    { TYPE_DC, CH1, FLD_YD, UNIT_WH, 24, 2, 1, false, 0 },
    { TYPE_DC, CH1, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH1, CMD_CALC, false, 3 },

    // DC Channel 2 (MPPT3) - offset 26
    { TYPE_DC, CH2, FLD_UDC, UNIT_V, 26, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_IDC, UNIT_A, 28, 2, 100, false, 2 },
    { TYPE_DC, CH2, FLD_PDC, UNIT_W, 30, 2, 10, false, 1 },
    { TYPE_DC, CH2, FLD_YT, UNIT_KWH, 32, 4, 1000, false, 3 },
    { TYPE_DC, CH2, FLD_YD, UNIT_WH, 36, 2, 1, false, 0 },
    { TYPE_DC, CH2, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH2, CMD_CALC, false, 3 },

    // DC Channel 3 (MPPT4) - offset 38
    { TYPE_DC, CH3, FLD_UDC, UNIT_V, 38, 2, 10, false, 1 },
    { TYPE_DC, CH3, FLD_IDC, UNIT_A, 40, 2, 100, false, 2 },
    { TYPE_DC, CH3, FLD_PDC, UNIT_W, 42, 2, 10, false, 1 },
    { TYPE_DC, CH3, FLD_YT, UNIT_KWH, 44, 4, 1000, false, 3 },
    { TYPE_DC, CH3, FLD_YD, UNIT_WH, 48, 2, 1, false, 0 },
    { TYPE_DC, CH3, FLD_IRR, UNIT_PCT, CALC_CH_IRR, CH3, CMD_CALC, false, 3 },

    // AC (3-phase) - offset 50
    { TYPE_AC, CH0, FLD_UAC, UNIT_V, 56, 2, 10, false, 1 },       // dummy, uses UAC_12
    { TYPE_AC, CH0, FLD_UAC_1N, UNIT_V, 50, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_UAC_2N, UNIT_V, 52, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_UAC_3N, UNIT_V, 54, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_UAC_12, UNIT_V, 56, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_UAC_23, UNIT_V, 58, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_UAC_31, UNIT_V, 60, 2, 10, false, 1 },
    { TYPE_AC, CH0, FLD_F, UNIT_HZ, 62, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W, 66, 2, 10, false, 1 },       // validated: 225.4W matched ~230W production
    { TYPE_AC, CH0, FLD_Q, UNIT_VAR, 64, 2, 10, true, 1 },        // validated: 0 VAR at PF=1.000
    { TYPE_AC, CH0, FLD_IAC, UNIT_A, CALC_TOTAL_IAC, 0, CMD_CALC, false, 2 },
    { TYPE_AC, CH0, FLD_IAC_1, UNIT_A, 72, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_IAC_2, UNIT_A, 74, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_IAC_3, UNIT_A, 76, 2, 100, false, 2 },
    { TYPE_AC, CH0, FLD_PF, UNIT_NONE, 78, 2, 1000, false, 3 },

    // Inverter - offset 80
    { TYPE_INV, CH0, FLD_T, UNIT_C, 80, 2, 10, true, 1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE, 82, 2, 1, false, 0 },

    // Calculated totals
    { TYPE_INV, CH0, FLD_YD, UNIT_WH, CALC_TOTAL_YD, 0, CMD_CALC, false, 0 },
    { TYPE_INV, CH0, FLD_YT, UNIT_KWH, CALC_TOTAL_YT, 0, CMD_CALC, false, 3 },
    { TYPE_INV, CH0, FLD_PDC, UNIT_W, CALC_TOTAL_PDC, 0, CMD_CALC, false, 1 },
    { TYPE_INV, CH0, FLD_EFF, UNIT_PCT, CALC_TOTAL_EFF, 0, CMD_CALC, false, 3 }
};

MIT_8CH::MIT_8CH(HoymilesRadio* radio, const uint64_t serial)
    : HMT_Abstract(radio, serial)
{
    // MIT-5000-8T returns a shorter SystemConfigPara response (38 bytes vs 48 for HMS/HMT)
    SystemConfigPara()->setExpectedByteCount(38);
}

bool MIT_8CH::isValidSerial(const uint64_t serial)
{
    // serial >= 0x152000000000 && serial <= 0x1520ffffffff
    uint16_t preSerial = (serial >> 32) & 0xffff;
    return preSerial == 0x1520;
}

String MIT_8CH::typeName() const
{
    return "MIT-5000-8T";
}

const byteAssign_t* MIT_8CH::getByteAssignment() const
{
    return byteAssignment;
}

uint8_t MIT_8CH::getByteAssignmentSize() const
{
    return sizeof(byteAssignment) / sizeof(byteAssignment[0]);
}

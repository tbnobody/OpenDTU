// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Bobby Noelte
 */
#include <cstring>
#include <string>

// OpenDTU
#include "Hoymiles.h"
#include "Configuration.h"
#include "Datastore.h"
#include "ModbusDtu.h"
#include "ModbusSettings.h"
#include "NetworkSettings.h"
#include "__compiled_constants.h"

// eModbus
#include "Logging.h"

// OpenDTU single phase (AN or AB) meter
// - FC 0x03 requests (read holding registers)
ModbusMessage OpenDTUMeter(ModbusMessage request) {
    uint16_t addr = 0;          // Start address
    uint16_t words = 0;         // # of words requested

    const CONFIG_T& config = Configuration.get();

    // read address from request
    request.get(2, addr);
    // read # of words from request
    request.get(4, words);

    uint16_t response_size = words * 2 + 6;
    ModbusDTUMessage response(response_size);     // The Modbus message we are going to give back

    LOG_D("Request FC03 0x%04x:%d - response with size %d\n", (int)addr, (int)words, (int)response_size);

    if (addr >= 40000) {
        // SunSpec - OpenDTU Meter

        // Set up response
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));

        // Complete response
        for (uint16_t reg = addr; reg < (addr + words); reg++) {
            if (reg < 40070) {
                // Model 1 - SunSpec Common Registers
                uint8_t reg_idx = reg - 40000;

                switch (reg_idx) {
                    case 0 ... 1:
                        // SunS
                        response.addString("SunS", reg_idx - 0);
                        break;
                    case 2:
                        // Model ID
                        response.addUInt16(1);
                        break;
                    case 3:
                        // SunSpec model register count (length without header (4))
                        response.addUInt16(66);
                        break;
                    case 4 ... 19:
                        // Manufacturer - string
                        response.addString("OpenDTU", reg_idx - 4);
                        break;
                    case 20 ... 35:
                        // Model - string
                        response.addString("OpenDTU Meter", reg_idx - 20);
                        break;
                    case 36 ... 43:
                        // Options - string
                        response.addString(config.Dev_PinMapping, reg_idx - 36);
                        break;
                    case 44 ... 51:
                        // Version - string
                        response.addString(__COMPILED_GIT_HASH__, reg_idx - 44);
                        break;
                    case 52 ... 67:
                        // Serial Number - string
                        response.addUInt64AsHexString(config.Dtu.Serial, reg_idx - 52);
                        break;
                    case 68:
                        // Device Address - uint16
                        response.addUInt16(request.getServerID());
                        break;
                    default:
                        // Pad
                        response.addUInt16(0x8000);
                        break;
                }
            } else if (reg < 40198) { // >= 40070
                // Model 211 - Single Phase (AN or AB) Meter FLOAT Model
                // The Meter acts as a virtual meter that combines the individual
                // measured values of the inverters, if useful.
                uint8_t reg_idx = reg - 40070;

                switch (reg_idx) {
                    case 0:
                        // Model ID
                        response.addUInt16(211);
                        break;
                    case 1:
                        // SunSpec model register count (length without model header (2))
                        response.addUInt16(124);
                        break;
                    case 28 ... 29:
                        // Watts (W), Total Real Power
                        response.addFloat32(Datastore.getTotalAcPowerEnabled() * -1, reg_idx - 28);
                        break;
                    case 60 ... 61:
                         // Total Watt-hours Exported (Wh), Total Real Energy Exported
                        response.addFloat32(Datastore.getTotalAcYieldTotalEnabled() * 1000, reg_idx - 60);
                        break;
                    case 124 ... 125:
                        // bitfield32
                        response.addUInt16(0);
                        break;
                    default:
                        // float32 - Not a Number
                        response.addFloat32(NAN, reg_idx & 0x01);
                        break;
                }
            } else if (reg < 40200) { // >= 40198
                // Mark end of models
                response.addUInt16(0);
            } else {
                goto address_error;
            }
        }
    } else {
address_error:
        // No valid regs - send respective error response
        LOG_W("Illegal data address 0x%04x:%d\n", (int)addr, (int)words);
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }

respond:
    HEXDUMP_D("Response FC03", response.data(), response.size());

    // Send response back
    return response;
}

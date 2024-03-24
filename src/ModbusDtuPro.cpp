// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Bobby Noelte
 */
#include <array>
#include <cstring>
#include <string>

// OpenDTU
#include "Hoymiles.h"
#include "Configuration.h"
#include "ModbusDtu.h"
#include "ModbusSettings.h"

// eModbus
#include "Logging.h"

// DTUPro - Start address of device SN register list
#define DTUPRO_ADDR_DEVICE_SN_LIST 0x2000

// DTUPro - Start address of microinverter data register list
#define DTUPRO_ADDR_INV_DATA_LIST 0x1000

// DTUPro - Number of Modbus registers per inverter port in inverter data list
#define DTUPRO_INV_DATA_REGISTER_COUNT 20

// DTUPro - Number of Modbus registers for serial number of inverter in SN list
#define DTUPRO_INV_SERIAL_REGISTER_COUNT 3

// DTUPro - Data type of a inverter register bank
#define DTUPRO_INV_DATA_TYPE_DEFAULT 0x3C

// DTUPro - Maximum number of inverter channels (ports) supported by DTU
#define DTUPRO_INV_CHANNEL_COUNT_MAX 99

#define DTUPRO_ALARM_CODE_OFFLINE 130

typedef struct {
    std::shared_ptr<InverterAbstract> inv; // inverter
    ChannelNum_t chan;  // dc channel 0 - 5 of inverter
} ModbusInvChannel_t;

// 3-Gen DTU-Pro
// - FC 0x03 requests (read holding registers)
ModbusMessage DTUPro(ModbusMessage request) {
    uint16_t addr = 0;          // Start address
    uint16_t words = 0;         // # of words requested

    uint8_t num_inverters = Hoymiles.getNumInverters();

    // read address from request
    request.get(2, addr);
    // read # of words from request
    request.get(4, words);

    uint16_t response_size = words * 2 + 6;
    ModbusDTUMessage response(response_size);     // The Modbus message we are going to give back

    LOG_D("Request FC03 0x%04x:%d - response with size %d\n", (int)addr, (int)words, (int)response_size);

    if (addr >= DTUPRO_ADDR_DEVICE_SN_LIST) {
        // Holding registers for serial numbers
        const CONFIG_T& config = Configuration.get();

        // Check for number of supported inverters
        // - add one virtual inverter with zero values to mark end of inverter list
        if ((addr + words) > (DTUPRO_ADDR_DEVICE_SN_LIST + (DTUPRO_INV_CHANNEL_COUNT_MAX + 1) * DTUPRO_INV_SERIAL_REGISTER_COUNT)) {
            // No valid regs - send respective error response
            LOG_W("Illegal data address 0x%04x:%d\n", (int)addr, (int)words);
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            return response;
        }

        // Set up response
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));

        // Complete response, device SN + final 0
        for (uint16_t reg = addr; reg < (addr + words); reg++) {
            if (reg >= (DTUPRO_ADDR_DEVICE_SN_LIST + (num_inverters + 1) * DTUPRO_INV_SERIAL_REGISTER_COUNT)) {
                // No more inverters, add 0 for end of inverters
                response.add((uint16_t)0);
            } else {
                // Inverter serial number
                uint64_t inv_serial = 0;
                uint8_t reg_idx;
                if (reg < (DTUPRO_ADDR_DEVICE_SN_LIST + DTUPRO_INV_SERIAL_REGISTER_COUNT)) {
                    // First slot is for DTU SN
                    inv_serial = config.Dtu.Serial;
                    reg_idx = reg - DTUPRO_ADDR_DEVICE_SN_LIST;
                } else {
                    // Starting from second slot inverter SNs are inserted
                    uint8_t inv_idx = (reg - DTUPRO_ADDR_DEVICE_SN_LIST) / DTUPRO_INV_SERIAL_REGISTER_COUNT - 1;
                    auto inv = Hoymiles.getInverterByPos(inv_idx);
                    if (inv != nullptr) {
                        inv_serial = inv->serial();
                    }
                    reg_idx = reg - (DTUPRO_ADDR_DEVICE_SN_LIST + (inv_idx + 1) * DTUPRO_INV_SERIAL_REGISTER_COUNT);
                }

                switch (reg_idx) {
                    case 0:
                        response.add((uint16_t)((inv_serial >> 32) & 0xFFFF));
                        break;
                    case 1:
                        response.add((uint16_t)((inv_serial >> 16) & 0xFFFF));
                        break;
                    case 2:
                        response.add((uint16_t)((inv_serial >> 0) & 0xFFFF));
                        break;
                    default:
                        response.add((uint16_t)0);
                        break;
                }
            }
        }
    } else if (addr >= DTUPRO_ADDR_INV_DATA_LIST) {
        // Loop all inverters and channels
        std::vector<ModbusInvChannel_t> channels;
        for (uint8_t inv_idx = 0; inv_idx < num_inverters; inv_idx++) {
            auto inv = Hoymiles.getInverterByPos(inv_idx);
            if (inv == nullptr) {
                LOG_W("Inverter at index %d not found\n", (int)inv_idx);
                continue;
            }

            auto inv_channels = inv->Statistics()->getChannelsByType(TYPE_DC);
            for (auto& inv_chan : inv_channels) {
                ModbusInvChannel_t channel;
                channel.inv = inv;
                channel.chan = inv_chan;
                channels.push_back(channel);

                // Debugging DC channel values
                LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(inv_chan + 1), "TYPE_DC.FLD_UDC",
                      inv->Statistics()->getChannelFieldValue(TYPE_DC, inv_chan, FLD_UDC));
                LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(inv_chan + 1), "TYPE_DC.FLD_IDC",
                      inv->Statistics()->getChannelFieldValue(TYPE_DC, inv_chan, FLD_IDC));
            }

            // Debugging AC channel values
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_UAC_1N",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC_1N));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_UAC_2N",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC_2N));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_UAC_3N",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC_3N));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_UAC",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_PAC",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_AC.FLD_F",
                  inv->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_F));
            // Debugging inverter channel values
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_INV.FLD_YD",
                  inv->Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_YD));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_INV.FLD_YT",
                  inv->Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_YT));
            LOG_D("Modbus inv#%d chan#%d %s: %f\n", (int)(inv_idx + 1), (int)(CH0 + 1), "TYPE_INV.FLD_T",
                  inv->Statistics()->getChannelFieldValue(TYPE_INV, CH0, FLD_T));
        }

        // Check for maximum number of channels supported by DTUPro
        // - One "virtual channel" added to mark end of list by zero values
        if ((addr + words) > (DTUPRO_ADDR_INV_DATA_LIST + (DTUPRO_INV_CHANNEL_COUNT_MAX + 1) * DTUPRO_INV_DATA_REGISTER_COUNT)) {
            // No valid regs - send respective error response
            LOG_W("Illegal data address 0x%04x:%d\n", (int)addr, (int)words);
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            goto respond;
        }

        // Set up response
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));

        // Complete response
        for (uint16_t reg = addr; reg < (addr + words); reg++) {
            uint16_t val;
            uint8_t chan_idx = (reg - DTUPRO_ADDR_INV_DATA_LIST) / DTUPRO_INV_DATA_REGISTER_COUNT;
            if (chan_idx >= channels.size()) {
                // Mark end of channel list
                val = response.addUInt16(0);
                LOG_D("response 0x%04x: 0x%04x\n", (unsigned int)reg, (unsigned int)val);
                continue;
            }

            uint8_t reg_idx = reg - (DTUPRO_ADDR_INV_DATA_LIST + chan_idx * DTUPRO_INV_DATA_REGISTER_COUNT);
            auto statistics = channels[chan_idx].inv->Statistics();

            switch (reg_idx) {
                case 0:
                    // Start of dataset
                    // Microinverter SN - digit 1,2
                    val = response.addUInt16(
                        (DTUPRO_INV_DATA_TYPE_DEFAULT << 8) + ((channels[chan_idx].inv->serial() >> 40) & 0x0FFUL));
                    break;
                case 1:
                    // Microinverter SN - digit 3,4,5,6
                    val = response.addUInt16(
                        (channels[chan_idx].inv->serial() >> 24) & 0x0FFFFUL);
                    break;
                case 2:
                    // Microinverter SN - digit 7,8,9,10
                    val = response.addUInt16(
                        (channels[chan_idx].inv->serial() >> 8) & 0x0FFFFUL);
                    break;
                case 3:
                    // Microinverter SN - digit 11,12
                    // Port number - starts at 1
                    val = response.addUInt16(
                        ((channels[chan_idx].inv->serial() << 8) & 0x0FF00) + channels[chan_idx].chan + 1);
                    break;
                case 4:
                    // PV Voltage (V) - decimal fixed point, precision 1
                    val = response.addFloatAsDecimalFixedPoint16(
                        statistics->getChannelFieldValue(TYPE_DC, channels[chan_idx].chan, FLD_UDC), 1);
                    break;
                case 5:
                    // PV Current (A) - decimal fixed point, precision 2
                    val = response.addFloatAsDecimalFixedPoint16(
                        statistics->getChannelFieldValue(TYPE_DC, channels[chan_idx].chan, FLD_IDC), 2);
                    break;
                case 6:
                    // Grid Voltage (V) - decimal fixed point, precision 1
                    if (statistics->hasChannelFieldValue(TYPE_AC, CH0, FLD_UAC_1N)) {
                        val = response.addFloatAsDecimalFixedPoint16(
                            statistics->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC_1N), 1);
                    } else {
                        val = response.addFloatAsDecimalFixedPoint16(
                            statistics->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC), 1);
                    }
                    break;
                case 7:
                    // Grid Frequency (Hz) - decimal fixed point, precision 2
                    val = response.addFloatAsDecimalFixedPoint16(
                        statistics->getChannelFieldValue(TYPE_AC, CH0, FLD_F), 2);
                    break;
                case 8:
                    // PV Power (W) - decimal fixed point, precision 1
                    val = response.addFloatAsDecimalFixedPoint16(
                        statistics->getChannelFieldValue(TYPE_DC, channels[chan_idx].chan, FLD_PDC), 1);
                    break;
                case 9:
                    // Today Production (Wh) - uint16
                    val = response.addUInt16((uint16_t)(statistics->getChannelFieldValue(TYPE_DC, channels[chan_idx].chan, FLD_YD) * 1));
                    break;
                case 10 ... 11:
                    // Total Production (Wh) - uint32
                    val = response.addUInt32(
                        (uint32_t)(statistics->getChannelFieldValue(TYPE_DC, channels[chan_idx].chan, FLD_YT) * 1000),
                        reg_idx - 10);
                    break;
                case 12:
                    // Temperature (°C) - decimal fixed point, precision 1
                    val = response.addFloatAsDecimalFixedPoint16(
                        statistics->getChannelFieldValue(TYPE_INV, CH0, FLD_T), 1);
                    break;
                case 13:
                    // Operating Status - TODO
                    val = response.addUInt16(3);
                    break;
                case 14:
                    // Alarm code
                    if (!channels[chan_idx].inv->isReachable()) {
                        val = DTUPRO_ALARM_CODE_OFFLINE;
                    } else if (statistics->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
                        uint8_t entry_count = channels[chan_idx].inv->EventLog()->getEntryCount();
                        if (entry_count > 0) {
                            AlarmLogEntry_t entry;
                            channels[chan_idx].inv->EventLog()->getLogEntry(entry_count - 1, entry);
                            val = entry.MessageId;
                        } else {
                            val = 0;
                        }
                    } else {
                        val = 0;
                    }
                    response.addUInt16(val);
                    break;
                case 15:
                    // Alarm count
                    if (!channels[chan_idx].inv->isReachable()) {
                        val = 1;
                    } else if (statistics->hasChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG)) {
                        val = channels[chan_idx].inv->EventLog()->getEntryCount();
                    } else {
                        val = 0;
                    }
                    response.addUInt16(val);
                    break;
                case 16:
                    // Link status - TODO
                    // Fixed - 0x07
                    val = response.addUInt16(0x0107);
                    break;
                case 17 ... 19:
                default:
                    // Reserved
                    val = response.addUInt16(0);
                    break;
            }
            LOG_D("response 0x%04x i%03u: 0x%04x\n", (unsigned int)reg, (unsigned int)reg_idx, (unsigned int)val);
        }
    } else {
        // No valid regs - send respective error response
        LOG_W("Illegal data address 0x%04x:%d\n", (int)addr, (int)words);
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        return response;
    }

respond:
    HEXDUMP_D("Response FC03", response.data(), response.size());

    // Send response back
    return response;
}

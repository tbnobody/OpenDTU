// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <vector>

#include <TaskSchedulerDeclarations.h>

// eModbus
#include "ModbusMessage.h"
#include "ModbusServerTCPasync.h"

class ModbusDTUMessage : public ModbusMessage {
private:
    // Value cache, mostly for conversion
    union Value {
        float val_float;
        int16_t val_i16;
        uint16_t val_u16;
        int32_t val_i32;
        uint32_t val_u32;
        uint64_t val_u64;
        uint32_t val_ip;
    } value;

    // Conversion cache
    union Conversion {
        // fixed point converted to u32
        uint32_t fixed_point_u32;
        // fixed point converted to u16
        uint16_t fixed_point_u16;
        // uint64 converted to hex string
        char u64_hex_str[sizeof(uint64_t) * 8 + 1];
        // ip address converted to String
        char ip_str[12];
    } conv;

public:
    // Default empty message Constructor - optionally takes expected size of MM_data
    explicit ModbusDTUMessage(uint16_t dataLen);

    // Special message Constructor - takes a std::vector<uint8_t>
    explicit ModbusDTUMessage(std::vector<uint8_t> s);

    // Add float to Modbus register
    uint16_t addFloat32(const float_t &val, const size_t reg_offset);

    // Add float as 32 bit decimal fixed point to Modbus register
    uint16_t addFloatAsDecimalFixedPoint32(const float_t &val, const float &precision, const size_t reg_offset);

    // Add float as 16 bit decimal fixed point to Modbus register
    uint16_t addFloatAsDecimalFixedPoint16(const float_t &val, const float &precision);

    // Add string to Modbus register
    uint16_t addString(const char * const str, const size_t length, const size_t reg_offset);

    // Add string to Modbus register
    uint16_t addString(const String &str, const size_t reg_offset);

    // Add uint16 to Modbus register
    uint16_t addUInt16(const uint16_t val);

    // Add uint32 to Modbus register
    uint16_t addUInt32(const uint32_t val, const size_t reg_offset);

    // Add uint64 to Modbus register
    uint16_t addUInt64(const uint64_t val, const size_t reg_offset);

    // Convert uint64 to hex string and add to Modbus register
    uint16_t addUInt64AsHexString(const uint64_t val, const size_t reg_offset);

    // Convert IP address to string and add to Modbus register
    uint16_t addIPAddressAsString(const IPAddress val, const size_t reg_offset);
};

ModbusMessage DTUPro(ModbusMessage request);
ModbusMessage OpenDTUTotal(ModbusMessage request);
ModbusMessage OpenDTUMeter(ModbusMessage request);

extern ModbusServerTCPasync ModbusTCPServer;

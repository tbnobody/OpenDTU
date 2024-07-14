// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Bobby Noelte
 */
#include <array>
#include <cstring>
#include <string>

// OpenDTU
#include "ModbusDtu.h"


ModbusDTUMessage::ModbusDTUMessage(uint16_t dataLen = 0) : ModbusMessage(dataLen) {
    value.val_float = NAN;
}

ModbusDTUMessage::ModbusDTUMessage(std::vector<uint8_t> s) : ModbusMessage(s) {
    value.val_float = NAN;
}

uint16_t ModbusDTUMessage::addFloat32(const float_t &val, const size_t reg_offset) {
    // Use union to convert from float to uint32
    value.val_float = val;

    return addUInt32(value.val_u32, reg_offset);
}

uint16_t ModbusDTUMessage::addFloatAsDecimalFixedPoint32(const float_t &val, const float &precision, const size_t reg_offset) {
    // Check if value is already converted to fixed point
    if (value.val_float != val) {
        // Multiply by 10^precision to shift the decimal point
        // Round the scaled value to the nearest integer
        // Use union to convert from fixed point to uint32
        value.val_i32 = round(val * std::pow(10, precision));
        // remember converted value
        conv.fixed_point_u32 = value.val_u32;
        // mark conversion
        value.val_float = val;
    }

    return addUInt32(conv.fixed_point_u32, reg_offset);
}

uint16_t ModbusDTUMessage::addFloatAsDecimalFixedPoint16(const float_t &val, const float &precision) {
    // Multiply by 10^precision to shift the decimal point
    // Round the scaled value to the nearest integer
    // Use union to convert from fixed point to uint16
    value.val_i16 = round(val * std::pow(10, precision));

    add(value.val_u16);
    return value.val_u16;
}

uint16_t ModbusDTUMessage::addString(const char * const str, const size_t length, const size_t reg_offset) {
    // Check if the position is within the bounds of the string
    size_t offset = reg_offset * sizeof(uint16_t);
    if (offset + sizeof(uint16_t) <= length) {
        // Reinterpret the memory at position 'offset' as uint16_t
        std::memcpy(&value.val_u16, str + offset, sizeof(uint16_t));
    } else {
        value.val_u16 = 0;
    }

    add(value.val_u16);
    return value.val_u16;
}

uint16_t ModbusDTUMessage::addString(const String &str, const size_t reg_offset) {
    return addString(str.c_str(), str.length(), reg_offset);
}

uint16_t ModbusDTUMessage::addUInt16(const uint16_t val) {
    add(val);
    return val;
}

uint16_t ModbusDTUMessage::addUInt32(const uint32_t val, const size_t reg_offset) {
    if (reg_offset <= 1) {
        value.val_u16 = val >> (16 * (1 - reg_offset));
    } else {
        value.val_u16 = 0;
    }
    add(value.val_u16);
    return value.val_u16;
}

uint16_t ModbusDTUMessage::addUInt64(const uint64_t val, const size_t reg_offset) {
    if (reg_offset <= 3) {
        value.val_u16 = val >> (16 * (3 - reg_offset));
    } else {
        value.val_u16 = 0;
    }
    add(value.val_u16);
    return value.val_u16;
}

uint16_t ModbusDTUMessage::addUInt64AsHexString(const uint64_t val, const size_t reg_offset) {
    // Check if value is already converted to hex string
    if (val != value.val_u64) {
        snprintf(&conv.u64_hex_str[0], sizeof(conv.u64_hex_str), "%0x%08x",
                ((uint32_t)((val >> 32) & 0xFFFFFFFFUL)),
                ((uint32_t)(val & 0xFFFFFFFFUL)));
        // mark conversion
        value.val_u64 = val;
    }

    return addString(&conv.u64_hex_str[0], sizeof(conv.u64_hex_str), reg_offset);
}

uint16_t ModbusDTUMessage::addIPAddressAsString(const IPAddress val, const size_t reg_offset) {
    // Check if value is already converted to hex string
    if (val != value.val_ip) {
        String str(val.toString());
        std::memcpy(&conv.ip_str, str.c_str(), std::min(sizeof(conv.ip_str), str.length()));
        // mark conversion
        value.val_ip = val;
    }

    return addString(&conv.ip_str[0], sizeof(conv.ip_str), reg_offset);
}

// Create server(s)
ModbusServerTCPasync ModbusTCPServer;

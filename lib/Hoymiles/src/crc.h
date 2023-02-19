// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

#define CRC8_INIT 0x00
#define CRC8_POLY 0x01

#define CRC16_MODBUS_POLYNOM 0xA001
#define CRC16_NRF24_POLYNOM 0x1021

uint8_t crc8(const uint8_t buf[], uint8_t len);
uint16_t crc16(const uint8_t buf[], uint8_t len, uint16_t start = 0xffff);
uint16_t crc16nrf24(const uint8_t buf[], uint16_t lenBits, uint16_t startBit = 0, uint16_t crcIn = 0xffff);

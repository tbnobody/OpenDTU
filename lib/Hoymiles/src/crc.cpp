// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "crc.h"

uint8_t crc8(const uint8_t buf[], const uint8_t len)
{
    uint8_t crc = CRC8_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc << 1) ^ ((crc & 0x80) ? CRC8_POLY : 0x00);
        }
    }
    return crc;
}

uint16_t crc16(const uint8_t buf[], const uint8_t len, const uint16_t start)
{
    uint16_t crc = start;
    uint8_t shift = 0;

    for (uint8_t i = 0; i < len; i++) {
        crc = crc ^ buf[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            shift = (crc & 0x0001);
            crc = crc >> 1;
            if (shift != 0)
                crc = crc ^ 0xA001;
        }
    }
    return crc;
}

uint16_t crc16nrf24(const uint8_t buf[], const uint16_t lenBits, const uint16_t startBit, const uint16_t crcIn)
{
    uint16_t crc = crcIn;
    uint8_t idx, val = buf[(startBit >> 3)];

    for (uint16_t bit = startBit; bit < lenBits; bit++) {
        idx = bit & 0x07;
        if (0 == idx)
            val = buf[(bit >> 3)];
        crc ^= 0x8000 & (val << (8 + idx));
        crc = (crc & 0x8000) ? ((crc << 1) ^ CRC16_NRF24_POLYNOM) : (crc << 1);
    }

    return crc;
}
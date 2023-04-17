// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

union serial_u {
    uint64_t u64;
    uint8_t b[8];
};

// maximum buffer length of packet received / sent to RF24 module
#define MAX_RF_PAYLOAD_SIZE 32

typedef struct {
    uint8_t mainCmd;
    uint8_t fragment[MAX_RF_PAYLOAD_SIZE];
    uint8_t len;
    uint8_t channel;
    int8_t rssi;
    bool wasReceived;
} fragment_t;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <cstdint>

typedef enum {
    CMD_OK,
    CMD_NOK,
    CMD_PENDING
} LastCommandSuccess;

class Parser {
public:
    uint32_t getLastUpdate();
    void setLastUpdate(uint32_t lastUpdate);

private:
    uint32_t _lastUpdate = 0;
};
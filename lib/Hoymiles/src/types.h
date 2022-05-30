#pragma once

#include <cstdint>

union serial_u {
    uint64_t u64;
    uint8_t b[8];
};
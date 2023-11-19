// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

class ResetReason {
public:
    static String get_reset_reason_verbose(uint8_t cpu_id);
    static String get_reset_reason_short(uint8_t cpu_id);
};
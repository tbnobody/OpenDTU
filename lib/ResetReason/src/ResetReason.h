// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

class ResetReasonClass {
public:
    String get_reset_reason_verbose(uint8_t cpu_id);
    String get_reset_reason_short(uint8_t cpu_id);
};

extern ResetReasonClass ResetReason;
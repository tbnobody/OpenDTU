// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

struct RadioRecoverySettings {
    uint32_t fragmentReplyTimeout;
    uint8_t fragmentRetryLimit;

    static constexpr uint32_t MIN_FRAGMENT_REPLY_TIMEOUT = 50;
    static constexpr uint32_t MAX_FRAGMENT_REPLY_TIMEOUT = 5000;
    static constexpr uint32_t MAX_FRAGMENT_RETRY_LIMIT = 50;

    // Zero selects the inverter model's default.
    static constexpr bool isValidFragmentReplyTimeout(const uint32_t value)
    {
        return value == 0 || (value >= MIN_FRAGMENT_REPLY_TIMEOUT && value <= MAX_FRAGMENT_REPLY_TIMEOUT);
    }

    static constexpr bool isValidFragmentRetryLimit(const uint32_t value)
    {
        return value <= MAX_FRAGMENT_RETRY_LIMIT;
    }
};

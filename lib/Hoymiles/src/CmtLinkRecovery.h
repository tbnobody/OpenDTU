// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

class InverterAbstract;

/**
 * Finds an unreachable CMT inverter again.
 *
 */
class CmtLinkRecovery {
public:
    static void onUnreachable(InverterAbstract* inv);

    static bool isHeard(const InverterAbstract* inv);

    static void onProbeHit(InverterAbstract* inv, const uint32_t frequency);

    static void enqueueChannelChange(InverterAbstract* inv, const uint32_t frequency = 0);

    static void onChannelChangeAccepted(InverterAbstract* inv, const uint8_t profile, const uint8_t startChannel);
    static void onChannelChangeRejected(InverterAbstract* inv, const uint8_t profile, const uint8_t startChannel);

private:
    static constexpr uint8_t PROBES_PER_CYCLE = 3;

    static constexpr uint8_t FAILS_BEFORE_RESCAN = 5;

    static constexpr uint8_t ACCEPTED_BEFORE_OFFSET_SWITCH = 3;

    static constexpr uint8_t MAX_MOVE_ATTEMPTS = 3;

    static constexpr int8_t GRID_OFFSET_ALTERNATIVE = 8;

    static constexpr uint32_t HEARD_POLL_CYCLES = 2;
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>

/**
 * Per inverter state of the sub-1GHz (CMT2300A) radio link.
 *

 */
struct CmtLinkState {

    uint32_t frequency = 0;


    uint16_t probeStep = 0;


    uint8_t failCount = 0;


    uint8_t profile = 0;
    uint8_t startChannel = 0;

    int8_t channelOffset = 0;

    uint8_t acceptedNoLink = 0;

    uint8_t moveAttempts = 0;

    uint32_t lastHeardMs = 0;
};

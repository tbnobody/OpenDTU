// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <vector>

/**
 * Calculates overscaling for power limiter inverters
 * Extracted from PowerLimiterOverscalingInverter to make it testable
 */
class OverscalingCalculator {
public:
    struct MpptData {
        float powerAC;  // AC power output of this MPPT
    };

    /**
     * Calculate the overscaled limit based on MPPT shading
     * @param currentLimitWatts Current power limit in watts
     * @param newExpectedOutputWatts New expected output power in watts
     * @param mpptData Vector of MPPT data with AC power outputs
     * @param inverterMaxPower Maximum inverter power in watts
     * @param currentThreshold Threshold factor for current limit (0.8 for low power, 0.97 for high power)
     * @param newThreshold Threshold factor for new limit (0.8 for low power, 0.97 for high power)
     * @return The calculated overscaled limit
     */
    static uint16_t calculateOverscaledLimit(uint16_t currentLimitWatts, uint16_t newExpectedOutputWatts,
                                           const std::vector<MpptData>& mpptData, uint16_t inverterMaxPower,
                                           float currentThreshold, float newThreshold);


    /**
     * Count shaded MPPTs based on expected power per MPPT
     * @param mpptData Vector of MPPT data
     * @param expectedPowerPerMppt Expected power per MPPT
     * @return Pair of (shaded count, total shaded power)
     */
    static std::pair<size_t, float> countShadedMppts(const std::vector<MpptData>& mpptData,
                                                     float expectedPowerPerMppt);
};

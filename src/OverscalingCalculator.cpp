#include "OverscalingCalculator.h"
#include <algorithm>
#include <cmath>

uint16_t OverscalingCalculator::calculateOverscaledLimit(uint16_t currentLimitWatts, uint16_t newExpectedOutputWatts,
                                                        const std::vector<MpptData>& mpptData, uint16_t inverterMaxPower,
                                                        float currentThreshold, float newThreshold) {
    // ============================================================================
    // OVERSCALING ALGORITHM
    // ============================================================================
    // Purpose: Compensate for shaded panels by increasing the total power limit
    //
    // How it works:
    // 1. Detect which MPPTs are shaded (producing less than expected)
    // 2. Calculate how much extra power non-shaded MPPTs can provide
    // 3. Distribute this extra power across all MPPTs to increase the total limit
    //
    // Note: This should not be used when Hoymiles 'Power Distribution Logic'
    // is available, as the inverter handles power distribution itself.
    // ============================================================================

    const size_t totalMppts = mpptData.size();

    // Calculate current actual output by summing MPPT power
    float currentActualOutput = 0.0f;
    for (const auto& mppt : mpptData) {
        currentActualOutput += mppt.powerAC;
    }

    // Calculate expected power per MPPT for both current and new limits
    const float currentExpectedPerMppt = (currentLimitWatts / totalMppts) * currentThreshold;
    const float newExpectedPerMppt = (newExpectedOutputWatts / totalMppts) * newThreshold;

    // ============================================================================
    // STEP 1: DETECT SHADING
    // ============================================================================
    // Determine which MPPTs are shaded based on power direction:
    // - Increasing power: Check current shading state (we know there's shading now)
    // - Decreasing power: Check new shading state (what we're transitioning to)
    //
    // Use actual current output to determine direction (handles overscaled limits)
    // Use current limit to calculate expected power per MPPT (for shading detection)

    const bool isIncreasingPower = newExpectedOutputWatts > currentActualOutput;
    size_t shadedMpptCount;
    float shadedMpptPowerSum;

    if (isIncreasingPower) {
        // When increasing power: only check current shading
        auto [count, powerSum] = countShadedMppts(mpptData, currentExpectedPerMppt);
        shadedMpptCount = count;
        shadedMpptPowerSum = powerSum;
    } else {
        // When decreasing power: only check new shading state
        auto [count, powerSum] = countShadedMppts(mpptData, newExpectedPerMppt);
        shadedMpptCount = count;
        shadedMpptPowerSum = powerSum;
    }

    // ============================================================================
    // STEP 2: VALIDATE SHADING CONDITIONS
    // ============================================================================

    // No shading detected - no overscaling needed
    if (shadedMpptCount == 0) {
        return newExpectedOutputWatts;
    }

    // All MPPTs are shaded - cannot compensate with overscaling
    const size_t nonShadedMpptCount = totalMppts - shadedMpptCount;
    if (nonShadedMpptCount == 0) {
        // If new limit is higher, apply it anyway (MPPTs might still produce more)
        // If new limit is lower, keep current limit (don't reduce unnecessarily)
        return (newExpectedOutputWatts > currentLimitWatts) ? newExpectedOutputWatts : currentLimitWatts;
    }

    // ============================================================================
    // STEP 3: CALCULATE OVERSCALED LIMIT
    // ============================================================================
    // Formula:
    // 1. Calculate how much power each non-shaded MPPT should provide
    // 2. Multiply by total MPPTs to get the new limit
    // 3. Apply inverter maximum power constraint

    // Calculate power per non-shaded MPPT
    const uint16_t powerPerNonShadedMppt = (newExpectedOutputWatts - shadedMpptPowerSum) / nonShadedMpptCount;

    // Calculate total overscaled limit
    uint16_t overscaledLimit = powerPerNonShadedMppt * totalMppts;

    // Apply inverter maximum power constraint
    overscaledLimit = std::min(overscaledLimit, inverterMaxPower);

    // ============================================================================
    // STEP 4: RETURN RESULT
    // ============================================================================
    // Only return overscaled limit if it's actually higher than expected
    return (overscaledLimit > newExpectedOutputWatts) ? overscaledLimit : newExpectedOutputWatts;
}


std::pair<size_t, float> OverscalingCalculator::countShadedMppts(const std::vector<MpptData>& mpptData,
                                                                float expectedPowerPerMppt) {
    // Count MPPTs that are producing less power than expected (shaded)
    size_t shadedCount = 0;
    float shadedPowerSum = 0.0f;

    for (const auto& mppt : mpptData) {
        if (mppt.powerAC < expectedPowerPerMppt) {
            shadedCount++;
            shadedPowerSum += mppt.powerAC;
        }
    }

    return {shadedCount, shadedPowerSum};
}

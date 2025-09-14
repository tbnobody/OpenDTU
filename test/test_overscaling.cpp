#include <iostream>
#include <cassert>
#include <vector>

// Include the actual OverscalingCalculator
#include "../include/OverscalingCalculator.h"

// Fixed thresholds as they don't have any effect on the tests
const float currentThreshold = 0.97f;
const float newThreshold = 0.97f;

const uint16_t inverterMaxPower = 2000;

void testShadedMpptCounting() {
    std::cout << "Testing: Shaded MPPT counting" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {100.0f}, {250.0f}, {300.0f}, {400.0f}
    };

    // Test with threshold of 250W
    auto [count, powerSum] = OverscalingCalculator::countShadedMppts(mpptData, 250.0f);

    assert(count == 1);  // First MPPT is below threshold
    assert(powerSum == 100.0f);

    std::cout << "✓ PASSED: Shaded MPPT counting correct" << std::endl;
}

// Tests for scenarios where new expected output is HIGHER than current limit
void testHigherOutputNoShading() {
    std::cout << "Testing: Higher output, no shading - no overscaling needed" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {250.0f}, {250.0f}, {250.0f}, {250.0f}  // All MPPTs at current limit (not shaded)
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(1000, 1500, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1500);  // Should return new expected output

    std::cout << "✓ PASSED: Higher output with no shading returns expected output" << std::endl;
}

void testHigherOutputOneShaded() {
    std::cout << "Testing: Higher output, one MPPT shaded - moderate overscaling" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {0.0f}, {125.0f}, {125.0f}, {125.0f}  // One MPPT shaded, others at current limit
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(500, 750, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1000);

    std::cout << "✓ PASSED: Higher output with 1 shaded MPPT scales to " << result << "W)" << std::endl;
}

void testHigherOutputMultipleShaded() {
    std::cout << "Testing: Higher output, multiple MPPTs shaded - significant overscaling" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {0.0f}, {0.0f}, {125.0f}, {125.0f}  // Two MPPTs shaded, others at current limit
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(500, 750, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1500);

    std::cout << "✓ PASSED: Higher output with multiple shaded MPPTs scales to " << result << "W)" << std::endl;
}

void testHigherOutputAllShaded() {
    std::cout << "Testing: Higher output, all MPPTs shaded - significant overscaling" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {0.0f}, {0.0f}, {0.0f}, {0.0f}  // All MPPTs shaded
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(500, 750, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    std::cout << "Result: " << result << "W)" << std::endl;
    assert(result == 750); // Use new limit anyway

    std::cout << "✓ PASSED: Higher output with multiple shaded MPPTs applies new limit" << std::endl;
}

// Tests for scenarios where new expected output is LOWER than current limit
void testLowerOutputNoShading() {
    std::cout << "Testing: Lower output, no shading - no overscaling needed" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {250.0f}, {250.0f}, {250.0f}, {250.0f}  // All MPPTs producing well, according to the new limit
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(1500, 1000, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1000);  // Should return new expected output

    std::cout << "✓ PASSED: Lower output with no shading returns expected output" << std::endl;
}

void testLowerOutputOneShaded() {
    std::cout << "Testing: Lower output, one MPPT shaded - moderate overscaling" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {0.0f}, {400.0f}, {400.0f}, {400.0f}  // One MPPT shaded, according to the new limit
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(1500, 1000, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1332);  // Should scale up moderately

    std::cout << "✓ PASSED: Lower output with 1 shaded MPPT scales moderately (result: " << result << "W)" << std::endl;
}

void testLowerOutputMultipleShaded() {
    std::cout << "Testing: Lower output, multiple MPPTs shaded - significant overscaling" << std::endl;

    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {0.0f}, {0.0f}, {500.0f}, {500.0f}  // Two MPPTs shaded, according to the new limit
    };

    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(1500, 800, mpptData, inverterMaxPower, currentThreshold, newThreshold);
    assert(result == 1600);

    std::cout << "✓ PASSED: Lower output with multiple shaded MPPTs scales to max (result: " << result << "W)" << std::endl;
}

// ============================================================================
// REALISTIC SHADING TESTS WITH NON-ZERO VALUES
// ============================================================================

void testRealisticShadingScenarios() {
    std::cout << "Testing: Realistic shading scenarios with partial power output" << std::endl;

    // Scenario 1: One MPPT partially shaded (producing 60% of expected) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData1 = {
        {300.0f}, {180.0f}, {300.0f}, {300.0f}  // One MPPT at 60% capacity
    };

    // Current limit: 1200W, Actual output: 1080W, New demand: 1300W (increasing)
    uint16_t result1 = OverscalingCalculator::calculateOverscaledLimit(1200, 1300, mpptData1, inverterMaxPower, currentThreshold, newThreshold);
    assert(result1 > 1300);  // Should apply overscaling since there's shading and increasing power

    // Scenario 2: Two MPPTs partially shaded (producing 70% and 50% of expected) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData2 = {
        {300.0f}, {210.0f}, {150.0f}, {300.0f}  // Two MPPTs at 70% and 50% capacity
    };

    // Current limit: 1200W, Actual output: 960W, New demand: 1400W (increasing)
    uint16_t result2 = OverscalingCalculator::calculateOverscaledLimit(1200, 1400, mpptData2, inverterMaxPower, currentThreshold, newThreshold);
    assert(result2 > 1400);  // Should apply overscaling since there's shading and increasing power

    // Scenario 3: All MPPTs partially shaded (producing 40-60% of expected) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData3 = {
        {120.0f}, {150.0f}, {180.0f}, {200.0f}  // All MPPTs at 40-60% capacity
    };

    // Current limit: 1200W, Actual output: 650W, New demand: 1000W (increasing)
    uint16_t result3 = OverscalingCalculator::calculateOverscaledLimit(1200, 1000, mpptData3, inverterMaxPower, currentThreshold, newThreshold);
    assert(result3 >= 1000);  // Should return at least the expected output

    std::cout << "✓ PASSED: Realistic shading scenarios handled correctly" << std::endl;
}

void testPartialShadingScenarios() {
    std::cout << "Testing: Partial shading scenarios with increasing power" << std::endl;

    // Scenario: One MPPT heavily shaded (30% capacity), others normal
    std::vector<OverscalingCalculator::MpptData> mpptData = {
        {400.0f}, {120.0f}, {400.0f}, {400.0f}  // One MPPT at 30% capacity
    };

    // Current limit: 1600W, Actual output: 1320W, New demand: 1800W
    uint16_t result = OverscalingCalculator::calculateOverscaledLimit(1600, 1800, mpptData, inverterMaxPower, currentThreshold, newThreshold);

    // Should apply overscaling since there's shading and we're increasing power
    assert(result > 1800);  // Should be overscaled
    assert(result <= 2000);  // Should not exceed inverter max

    std::cout << "✓ PASSED: Partial shading with increasing power scales correctly (result: " << result << "W)" << std::endl;
}

void testEdgeCaseShadingScenarios() {
    std::cout << "Testing: Edge case shading scenarios" << std::endl;

    // Scenario 1: MPPTs producing exactly at threshold (borderline shading) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData1 = {
        {291.0f}, {291.0f}, {291.0f}, {291.0f}  // Exactly at 97% of 300W expected
    };

    // Current limit: 1200W, Actual output: 1164W, New demand: 1300W (increasing)
    uint16_t result1 = OverscalingCalculator::calculateOverscaledLimit(1200, 1300, mpptData1, inverterMaxPower, currentThreshold, newThreshold);
    assert(result1 == 1300);  // Should not be considered shaded (exactly at threshold)

    // Scenario 2: MPPTs just below threshold (minimal shading) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData2 = {
        {290.0f}, {290.0f}, {290.0f}, {290.0f}  // Just below 97% of 300W expected
    };

    // Current limit: 1200W, Actual output: 1160W, New demand: 1300W (increasing)
    uint16_t result2 = OverscalingCalculator::calculateOverscaledLimit(1200, 1300, mpptData2, inverterMaxPower, currentThreshold, newThreshold);
    assert(result2 == 1300);  // Should return new limit since it's higher than current limit

    // Scenario 3: Mixed shading levels (some heavily shaded, some moderately) - increasing power
    std::vector<OverscalingCalculator::MpptData> mpptData3 = {
        {400.0f}, {100.0f}, {250.0f}, {350.0f}  // Mixed shading levels
    };

    // Current limit: 1600W, Actual output: 1100W, New demand: 1400W (increasing)
    uint16_t result3 = OverscalingCalculator::calculateOverscaledLimit(1600, 1400, mpptData3, inverterMaxPower, currentThreshold, newThreshold);
    assert(result3 > 1400);  // Should apply overscaling since there's shading and increasing power

    std::cout << "✓ PASSED: Edge case shading scenarios handled correctly" << std::endl;
}

int main() {
    std::cout << "=== OpenDTU-OnBattery Overscaling Calculator Tests ===" << std::endl;
    std::cout << "This tests the actual overscaling logic in isolation" << std::endl;
    std::cout << std::endl;

    try {
        testShadedMpptCounting();

        testHigherOutputNoShading();
        testHigherOutputOneShaded();
        testHigherOutputMultipleShaded();
        testHigherOutputAllShaded();

        testLowerOutputNoShading();
        testLowerOutputOneShaded();
        testLowerOutputMultipleShaded();
        // This test case is missing because its not valid, when all MPPTs are shaded, it will be an increase, not a decrease
        // testLowerOutputAllShaded();

        // Realistic shading tests with non-zero values
        testRealisticShadingScenarios();
        testPartialShadingScenarios();
        testEdgeCaseShadingScenarios();

        std::cout << std::endl;
        std::cout << "✓ ALL TESTS PASSED!" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "❌ TEST FAILED: Unknown error" << std::endl;
        return 1;
    }
}

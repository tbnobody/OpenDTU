#include "PowerLimiterOverscalingInverter.h"
#include "OverscalingCalculator.h"
#include <LogHelper.h>

#undef TAG
static const char* TAG = "dynamicPowerLimiter";
#define SUBTAG _logPrefix

PowerLimiterOverscalingInverter::PowerLimiterOverscalingInverter(PowerLimiterInverterConfig const& config)
    : PowerLimiterInverter(config) { }

float PowerLimiterOverscalingInverter::calculateRequiredOutputThreshold(uint16_t limitWatts) const
{
    // above 15% we can apply our simple 97% percent rule
    float threshold = 0.97;

    // if the limit is 15% or below, we use a lower threshold
    // of 80% to compensate for the lower efficiency at low power.
    if (limitWatts <= getInverterMaxPowerWatts() * 0.15) {
        threshold = 0.8;
    }

    return threshold;
}

float PowerLimiterOverscalingInverter::calculateMpptPowerAC(MpptNum_t mppt) const
{
    auto pStats = _spInverter->Statistics();
    float inverterEfficiencyFactor = pStats->getChannelFieldValue(TYPE_INV, CH0, FLD_EFF) / 100;
    float mpptPowerAC = 0.0;

    std::vector<ChannelNum_t> mpptChnls = _spInverter->getChannelsDCByMppt(mppt);
    for (auto& c : mpptChnls) {
        mpptPowerAC += pStats->getChannelFieldValue(TYPE_DC, c, FLD_PDC) * inverterEfficiencyFactor;
    }

    return mpptPowerAC;
}

uint16_t PowerLimiterOverscalingInverter::applyIncrease(uint16_t increase)
{
    if (!isEligible()) { return 0; }

    if (increase == 0) { return 0; }

    // do not wake inverter up if it would produce too much power
    if (!isProducing() && _config.LowerPowerLimit > increase) { return 0; }

    auto actualIncrease = std::min(increase, getMaxIncreaseWatts());
    setAcOutput(getCurrentOutputAcWatts() + actualIncrease);
    return actualIncrease;
}

uint16_t PowerLimiterOverscalingInverter::scaleLimit(uint16_t newExpectedOutputWatts)
{
    // overscalling allows us to compensate for shaded panels by increasing the
    // total power limit, if the inverter is solar powered.
    // this feature should not be used when homyiles 'Power Distribution Logic' is available
    // as the inverter will take care of the power distribution across the MPPTs itself.
    // (added in inverter firmware 01.01.12 on supported models (HMS-1600/1800/2000))
    // When disabled we return the expected output.
    if (!overscalingEnabled()) { return newExpectedOutputWatts; }

    // prevent scaling if inverter is not producing, as input channels are not
    // producing energy and hence are detected as not-producing, causing
    // unreasonable scaling.
    if (!isProducing()) { return newExpectedOutputWatts; }

    std::vector<ChannelNum_t> dcChnls = _spInverter->getChannelsDC();
    std::vector<MpptNum_t> dcMppts = _spInverter->getMppts();
    size_t dcTotalChnls = dcChnls.size();
    size_t dcTotalMppts = dcMppts.size();

    // if there is only one MPPT available, there is nothing we can do
    if (dcTotalMppts <= 1) { return newExpectedOutputWatts; }

    // test for a reasonable power limit that allows us to assume that an input
    // channel with little energy is actually not producing, rather than
    // producing very little due to the very low limit.
    if (getCurrentLimitWatts() < dcTotalChnls * 10) { return newExpectedOutputWatts; }

    // Convert MPPT data to the format expected by OverscalingCalculator
    std::vector<OverscalingCalculator::MpptData> mpptData;
    for (auto& m : dcMppts) {
        OverscalingCalculator::MpptData mppt;
        mppt.powerAC = calculateMpptPowerAC(m);
        mpptData.push_back(mppt);
    }

    // Calculate thresholds for current and new limits
    float currentThreshold = calculateRequiredOutputThreshold(getCurrentLimitWatts());
    float newThreshold = calculateRequiredOutputThreshold(newExpectedOutputWatts);

    // Calculate overscaled limit using the extracted calculator
    uint16_t overScaledLimit = OverscalingCalculator::calculateOverscaledLimit(
        getCurrentLimitWatts(), newExpectedOutputWatts, mpptData, getInverterMaxPowerWatts(),
        currentThreshold, newThreshold);

    if (overScaledLimit <= newExpectedOutputWatts) {
        return newExpectedOutputWatts;
    }

    // Count newly shaded MPPTs for logging
    float newExpectedMpptPowerAc = (newExpectedOutputWatts / dcTotalMppts) * newThreshold;

    size_t newlyShadedMpptCount = 0;
    for (auto& m : dcMppts) {
        float mpptPowerAC = calculateMpptPowerAC(m);
        if (mpptPowerAC < newExpectedMpptPowerAc) {
            newlyShadedMpptCount++;
        }
    }

    DTU_LOGD("%d/%d mppts are not-producing/shaded, scaling %d W",
            newlyShadedMpptCount, dcTotalMppts, overScaledLimit);

    return overScaledLimit;
}

void PowerLimiterOverscalingInverter::setAcOutput(uint16_t expectedOutputWatts)
{
    // make sure to enforce the lower and upper bounds
    expectedOutputWatts = std::min(expectedOutputWatts, getConfiguredMaxPowerWatts());
    expectedOutputWatts = std::max(expectedOutputWatts, _config.LowerPowerLimit);

    setExpectedOutputAcWatts(expectedOutputWatts);
    setTargetPowerLimitWatts(scaleLimit(expectedOutputWatts));
    setTargetPowerState(true);
}

bool PowerLimiterOverscalingInverter::overscalingEnabled() const
{
    return _config.UseOverscaling && !_spInverter->supportsPowerDistributionLogic();
}

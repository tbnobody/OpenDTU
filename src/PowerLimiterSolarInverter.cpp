#include "PowerLimiterSolarInverter.h"

PowerLimiterSolarInverter::PowerLimiterSolarInverter(PowerLimiterInverterConfig const& config)
    : PowerLimiterOverscalingInverter(config) { }

uint16_t PowerLimiterSolarInverter::getExpectedOutputAcWatts() const
{
    // We only return the expected output if the inverter is producing.
    // This is to avoid that solar-powered inverters are counted in with
    // higher power than they are actually able to produce during a increase,
    // which can cause the DPL to request less power from the smart-buffer or
    // battery-powered inverters or even switch them off. This is especially
    // critical during sunset and sunrise, when solar-powered inverters
    // become reachable/unreachable a couple of times but can actually not
    // produce any power at all.
    if (!isProducing()) { return 0; }

    return PowerLimiterInverter::getExpectedOutputAcWatts();
}

uint16_t PowerLimiterSolarInverter::getMaxReductionWatts(bool) const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) { return 0; }

    auto low = std::min(getCurrentLimitWatts(), getCurrentOutputAcWatts());
    if (low <= _config.LowerPowerLimit) { return 0; }

    return getCurrentOutputAcWatts() - _config.LowerPowerLimit;
}

uint16_t PowerLimiterSolarInverter::getMaxIncreaseWatts() const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) {
        // the inverter is not producing, we don't know how much we can increase
        // the power, but its likely that its early in the morning when this happens,
        // so we return the lower power limit.
        return _config.LowerPowerLimit;
    }

    // The inverter produces the configured max power or more.
    if (getCurrentOutputAcWatts() >= getConfiguredMaxPowerWatts()) { return 0; }

    // The limit is already at the max or higher.
    if (getCurrentLimitWatts() >= getInverterMaxPowerWatts()) { return 0; }

    // when overscaling is NOT enabled and the limit is already at the configured max power or higher,
    // we can't increase the power.
    if (!overscalingEnabled() && getCurrentLimitWatts() >= getConfiguredMaxPowerWatts()) { return 0; }

    uint16_t inverterMaxLimit = 0;

    if (overscalingEnabled() || _spInverter->supportsPowerDistributionLogic()) {
        // we use the inverter's max power, because each MPPT can deliver its max power individually
        inverterMaxLimit = getInverterMaxPowerWatts();
    } else {
        inverterMaxLimit = getConfiguredMaxPowerWatts();
    }

    std::vector<MpptNum_t> dcMppts = _spInverter->getMppts();
    size_t totalMppts = dcMppts.size();

    float requiredOutputThreshold = calculateRequiredOutputThreshold(getCurrentLimitWatts());
    float expectedAcPowerPerMppt = (getCurrentLimitWatts() / totalMppts) * requiredOutputThreshold;
    uint16_t maxPowerPerMppt = inverterMaxLimit / totalMppts;
    uint16_t nonShadedMaxIncrease = 0;
    size_t nonLimitedMppts = 0;

    for (auto& m : dcMppts) {
        float mpptPowerAC = calculateMpptPowerAC(m);

        if (mpptPowerAC >= expectedAcPowerPerMppt) {
            if (maxPowerPerMppt > mpptPowerAC) {
                nonShadedMaxIncrease += maxPowerPerMppt - mpptPowerAC;
                nonLimitedMppts++;
            }
        }
    }

    if (nonLimitedMppts == 0) {
        // all mppts are running at the max power or are shaded, we can't increase the power
        return 0;
    }

    uint16_t maxOutputIncrease = getConfiguredMaxPowerWatts() - getCurrentOutputAcWatts();
    uint16_t maxLimitIncrease = inverterMaxLimit - getCurrentLimitWatts();

    // when overscaling is disabled and PDL is not supported,
    // we must reduce the max limit increase because the limit will be divided
    // across all mppts.
    if (!_config.UseOverscaling && !_spInverter->supportsPowerDistributionLogic()) {
        maxLimitIncrease = (maxLimitIncrease / totalMppts) * nonLimitedMppts;
    }

    // find the max total increase
    uint16_t maxTotalIncrease = std::min(maxOutputIncrease, maxLimitIncrease);

    // calculated increase should not exceed the max total increase
    return std::min(maxTotalIncrease, nonShadedMaxIncrease);
}

uint16_t PowerLimiterSolarInverter::applyReduction(uint16_t reduction, bool)
{
    if (!isEligible()) { return 0; }

    if (reduction == 0) { return 0; }

    uint16_t currentOutputAcWatts = getCurrentOutputAcWatts();

    if ((currentOutputAcWatts - _config.LowerPowerLimit) >= reduction) {
        setAcOutput(currentOutputAcWatts - reduction);
        return reduction;
    }

    setAcOutput(_config.LowerPowerLimit);
    return currentOutputAcWatts - _config.LowerPowerLimit;
}

uint16_t PowerLimiterSolarInverter::standby()
{
    // solar-powered inverters are never actually put into standby (by the
    // DPL), but only set to the configured lower power limit instead.
    setAcOutput(_config.LowerPowerLimit);
    return getCurrentOutputAcWatts() - _config.LowerPowerLimit;
}

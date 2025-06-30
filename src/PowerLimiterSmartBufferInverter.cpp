#include "PowerLimiterSmartBufferInverter.h"

PowerLimiterSmartBufferInverter::PowerLimiterSmartBufferInverter(PowerLimiterInverterConfig const& config)
    : PowerLimiterOverscalingInverter(config) { }

uint16_t PowerLimiterSmartBufferInverter::getMaxReductionWatts(bool allowStandby) const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) { return 0; }

    if (allowStandby && _config.AllowStandby) { return getCurrentOutputAcWatts(); }

    if (getCurrentOutputAcWatts() <= _config.LowerPowerLimit) { return 0; }

    return getCurrentOutputAcWatts() - _config.LowerPowerLimit;
}

uint16_t PowerLimiterSmartBufferInverter::getMaxIncreaseWatts() const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) {
        return getConfiguredMaxPowerWatts();
    }

    // The inverter can produce more than the set limit and as such
    // also more than the configured max power.
    if (getCurrentOutputAcWatts() >= getConfiguredMaxPowerWatts()) { return 0; }

    // The limit is already at the max or higher.
    if (getCurrentLimitWatts() >= getInverterMaxPowerWatts()) { return 0; }

    // when overscaling is in use we must not substract the current limit
    // because it might be scaled and higher than the configured max power.
    if (overscalingEnabled()) {
        uint16_t maxOutputIncrease = getConfiguredMaxPowerWatts() - getCurrentOutputAcWatts();
        uint16_t maxLimitIncrease = getInverterMaxPowerWatts() - getCurrentLimitWatts();

        // constrains the increase to the limit of the inverter.
        return std::min(maxOutputIncrease, maxLimitIncrease);
    }

    // this should not happen, but we want to
    // be robust in case something else set a limit on the inverter (or in
    // case we did something wrong...) or overscaling was in use but then disabled.
    if (getCurrentLimitWatts() >= getConfiguredMaxPowerWatts()) { return 0; }

    // we must not substract the current AC output here, but the current
    // limit value, so we avoid trying to produce even more even if the
    // inverter is already at the maximum limit value (the actual AC
    // output may be less than the inverter's current power limit).
    return getConfiguredMaxPowerWatts() - getCurrentLimitWatts();
}

uint16_t PowerLimiterSmartBufferInverter::applyReduction(uint16_t reduction, bool allowStandby)
{
    if (!isEligible()) { return 0; }

    if (reduction == 0) { return 0; }

    uint16_t currentOutputAcWatts = getCurrentOutputAcWatts();

    auto low = std::min(getCurrentLimitWatts(), currentOutputAcWatts);
    if (low <= _config.LowerPowerLimit) {
        if (allowStandby && _config.AllowStandby) {
            standby();
            return std::min(reduction, currentOutputAcWatts);
        }
        return 0;
    }

    if ((currentOutputAcWatts - _config.LowerPowerLimit) >= reduction) {
        setAcOutput(currentOutputAcWatts - reduction);
        return reduction;
    }

    if (allowStandby && _config.AllowStandby) {
        standby();
        return std::min(reduction, currentOutputAcWatts);
    }

    setAcOutput(_config.LowerPowerLimit);
    return currentOutputAcWatts - _config.LowerPowerLimit;
}

uint16_t PowerLimiterSmartBufferInverter::standby()
{
    setTargetPowerState(false);
    setExpectedOutputAcWatts(0);
    return getCurrentOutputAcWatts();
}

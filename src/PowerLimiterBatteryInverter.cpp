#include "PowerLimiterBatteryInverter.h"

PowerLimiterBatteryInverter::PowerLimiterBatteryInverter(bool verboseLogging, PowerLimiterInverterConfig const& config)
    : PowerLimiterInverter(verboseLogging, config) { }

uint16_t PowerLimiterBatteryInverter::getMaxReductionWatts(bool allowStandby) const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) { return 0; }

    if (allowStandby) { return getCurrentOutputAcWatts(); }

    if (getCurrentOutputAcWatts() <= _config.LowerPowerLimit) { return 0; }

    return getCurrentOutputAcWatts() - _config.LowerPowerLimit;
}

uint16_t PowerLimiterBatteryInverter::getMaxIncreaseWatts() const
{
    if (!isEligible()) { return 0; }

    if (!isProducing()) {
        return getConfiguredMaxPowerWatts();
    }

    // this should not happen for battery-powered inverters, but we want to
    // be robust in case something else set a limit on the inverter (or in
    // case we did something wrong...).
    if (getCurrentLimitWatts() >= getConfiguredMaxPowerWatts()) { return 0; }

    // we must not substract the current AC output here, but the current
    // limit value, so we avoid trying to produce even more even if the
    // inverter is already at the maximum limit value (the actual AC
    // output may be less than the inverter's current power limit).
    return getConfiguredMaxPowerWatts() - getCurrentLimitWatts();
}

uint16_t PowerLimiterBatteryInverter::applyReduction(uint16_t reduction, bool allowStandby)
{
    if (!isEligible()) { return 0; }

    if (reduction == 0) { return 0; }

    auto low = std::min(getCurrentLimitWatts(), getCurrentOutputAcWatts());
    if (low <= _config.LowerPowerLimit) {
        if (allowStandby) {
            standby();
            return std::min(reduction, getCurrentOutputAcWatts());
        }
        return 0;
    }

    if ((getCurrentLimitWatts() - _config.LowerPowerLimit) >= reduction) {
        setAcOutput(getCurrentLimitWatts() - reduction);
        return reduction;
    }

    if (allowStandby) {
        standby();
        return std::min(reduction, getCurrentOutputAcWatts());
    }

    setAcOutput(_config.LowerPowerLimit);
    return getCurrentOutputAcWatts() - _config.LowerPowerLimit;
}

uint16_t PowerLimiterBatteryInverter::applyIncrease(uint16_t increase)
{
    if (!isEligible()) { return 0; }

    if (increase == 0) { return 0; }

    // do not wake inverter up if it would produce too much power
    if (!isProducing() && _config.LowerPowerLimit > increase) { return 0; }

    auto baseline = getCurrentLimitWatts();

    // battery-powered inverters in standby can have an arbitrary limit, yet
    // the baseline is 0 in case we are about to wake it up from standby.
    if (!isProducing()) { baseline = 0; }

    auto actualIncrease = std::min(increase, getMaxIncreaseWatts());
    setAcOutput(baseline + actualIncrease);
    return actualIncrease;
}

uint16_t PowerLimiterBatteryInverter::standby()
{
    setTargetPowerState(false);
    setExpectedOutputAcWatts(0);
    return getCurrentOutputAcWatts();
}

void PowerLimiterBatteryInverter::setAcOutput(uint16_t expectedOutputWatts)
{
    // make sure to enforce the lower and upper bounds
    expectedOutputWatts = std::min(expectedOutputWatts, getConfiguredMaxPowerWatts());
    expectedOutputWatts = std::max(expectedOutputWatts, _config.LowerPowerLimit);

    setExpectedOutputAcWatts(expectedOutputWatts);
    setTargetPowerLimitWatts(expectedOutputWatts);
    setTargetPowerState(true);
}

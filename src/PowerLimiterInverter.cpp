#include "RestartHelper.h"
#include "MessageOutput.h"
#include "PowerLimiterInverter.h"
#include "PowerLimiterBatteryInverter.h"
#include "PowerLimiterSolarInverter.h"
#include "inverters/HMS_4CH.h"

std::unique_ptr<PowerLimiterInverter> PowerLimiterInverter::create(
        bool verboseLogging, PowerLimiterInverterConfig const& config)
{
    std::unique_ptr<PowerLimiterInverter> upInverter;

    if (config.IsSolarPowered) {
        upInverter = std::make_unique<PowerLimiterSolarInverter>(verboseLogging, config);
    }
    else {
        upInverter = std::make_unique<PowerLimiterBatteryInverter>(verboseLogging, config);
    }

    if (nullptr == upInverter->_spInverter) { return nullptr; }

    return std::move(upInverter);
}

PowerLimiterInverter::PowerLimiterInverter(bool verboseLogging, PowerLimiterInverterConfig const& config)
    : _config(config)
    , _verboseLogging(verboseLogging)
{
    _spInverter = Hoymiles.getInverterBySerial(config.Serial);
    if (!_spInverter) { return; }

    snprintf(_serialStr, sizeof(_serialStr), "%0x%08x",
            static_cast<uint32_t>((config.Serial >> 32) & 0xFFFFFFFF),
            static_cast<uint32_t>(config.Serial & 0xFFFFFFFF));

    snprintf(_logPrefix, sizeof(_logPrefix), "[DPL inverter %s]:", _serialStr);
}

bool PowerLimiterInverter::isEligible() const
{
    if (!isReachable() || !isSendingCommandsEnabled()) { return false; }

    // after startup, the limit effective at the inverter is not known. the
    // respective message to request this info is only sent after a significant
    // backoff (4 minutes). this is to avoid error messages to appear in the
    // inverter's event log. we will wait until the current limit is known.
    if (getCurrentLimitWatts() == 0) { return false; }

    // the model-dependent maximum AC power output is only known after the
    // first DevInfoSimpleCommand succeeded. we desperately need this info, so
    // the inverter is not eligible until this value is known.
    if (getInverterMaxPowerWatts() == 0) { return false; }

    return true;
}

bool PowerLimiterInverter::update()
{
    auto reset = [this]() -> bool {
        _oTargetPowerState = std::nullopt;
        _oTargetPowerLimitWatts = std::nullopt;
        _oUpdateStartMillis = std::nullopt;
        return false;
    };

    // do not reset _updateTimeouts below if no state change requested
    if (!_oTargetPowerState.has_value() && !_oTargetPowerLimitWatts.has_value()) {
        return reset();
    }

    if (!_oUpdateStartMillis.has_value()) {
        _oUpdateStartMillis = millis();
    }

    if ((millis() - *_oUpdateStartMillis) > 30 * 1000) {
        ++_updateTimeouts;
        MessageOutput.printf("%s timeout (%d in succession), "
                "state transition pending: %s, limit pending: %s\r\n",
                _logPrefix, _updateTimeouts,
                (_oTargetPowerState.has_value()?"yes":"no"),
                (_oTargetPowerLimitWatts.has_value()?"yes":"no"));

        // NOTE that this is not always 5 minutes, since this counts timeouts,
        // not absolute time. after any timeout, an update cycle ends. a new
        // timeout can only happen after starting a new update cycle, which in
        // turn is only started if the DPL did calculate a new limit, which in
        // turn does not happen while the inverter is unreachable, no matter
        // how long (a whole night) that might be.
        if (_updateTimeouts >= 10) {
            MessageOutput.printf("%s issuing restart command after update "
                    "timed out repeatedly\r\n", _logPrefix);
            _spInverter->sendRestartControlRequest();
        }

        if (_updateTimeouts >= 20) {
            MessageOutput.printf("%s restarting system since inverter is "
                    "unresponsive\r\n", _logPrefix);
            RestartHelper.triggerRestart();
        }

        return reset();
    }

    auto constexpr halfOfAllMillis = std::numeric_limits<uint32_t>::max() / 2;

    auto switchPowerState = [this](bool transitionOn) -> bool {
        // no power state transition requested at all
        if (!_oTargetPowerState.has_value()) { return false; }

        // the transition that may be started is not the one which is requested
        if (transitionOn != *_oTargetPowerState) { return false; }

        // wait for pending power command(s) to complete
        auto lastPowerCommandState = _spInverter->PowerCommand()->getLastPowerCommandSuccess();
        if (CMD_PENDING == lastPowerCommandState) {
            return true;
        }

        // we need to wait for statistics that are more recent than
        // the last power update command to reliably use isProducing()
        auto lastPowerCommandMillis = _spInverter->PowerCommand()->getLastUpdateCommand();
        auto lastStatisticsMillis = _spInverter->Statistics()->getLastUpdate();
        if ((lastStatisticsMillis - lastPowerCommandMillis) > halfOfAllMillis) { return true; }

        if (isProducing() != *_oTargetPowerState) {
            MessageOutput.printf("%s %s inverter...\r\n", _logPrefix,
                    ((*_oTargetPowerState)?"Starting":"Stopping"));
            _spInverter->sendPowerControlRequest(*_oTargetPowerState);
            return true;
        }

        _oTargetPowerState = std::nullopt; // target power state reached
        return false;
    };

    // we use a lambda function here to be able to use return statements,
    // which allows to avoid if-else-indentions and improves code readability
    auto updateLimit = [this]() -> bool {
        // no limit update requested at all
        if (!_oTargetPowerLimitWatts.has_value()) { return false; }

        // wait for pending limit command(s) to complete
        auto lastLimitCommandState = _spInverter->SystemConfigPara()->getLastLimitCommandSuccess();
        if (CMD_PENDING == lastLimitCommandState) {
            return true;
        }

        float newRelativeLimit = static_cast<float>(*_oTargetPowerLimitWatts * 100) / getInverterMaxPowerWatts();

        // if no limit command is pending, the SystemConfigPara does report the
        // current limit, as the answer by the inverter to a limit command is
        // the canonical source that updates the known current limit.
        auto currentRelativeLimit = _spInverter->SystemConfigPara()->getLimitPercent();

        // we assume having exclusive control over the inverter. if the last
        // limit command was successful and sent after we started the last
        // update cycle, we should assume *our* requested limit was set.
        uint32_t lastLimitCommandMillis = _spInverter->SystemConfigPara()->getLastUpdateCommand();
        if ((lastLimitCommandMillis - *_oUpdateStartMillis) < halfOfAllMillis &&
                CMD_OK == lastLimitCommandState) {
            MessageOutput.printf("%s actual limit is %.1f %% (%.0f W "
                    "respectively), effective %d ms after update started, "
                    "requested were %.1f %%\r\n",
                    _logPrefix, currentRelativeLimit,
                    (currentRelativeLimit * getInverterMaxPowerWatts() / 100),
                    (lastLimitCommandMillis - *_oUpdateStartMillis),
                    newRelativeLimit);

            if (std::abs(newRelativeLimit - currentRelativeLimit) > 2.0) {
                MessageOutput.printf("%s NOTE: expected limit of %.1f %% "
                        "and actual limit of %.1f %% mismatch by more than 2 %%, "
                        "is the DPL in exclusive control over the inverter?\r\n",
                        _logPrefix, newRelativeLimit, currentRelativeLimit);
            }

            _oTargetPowerLimitWatts = std::nullopt;
            return false;
        }

        MessageOutput.printf("%s sending limit of %.1f %% (%.0f W "
                "respectively), max output is %d W\r\n", _logPrefix,
                newRelativeLimit, (newRelativeLimit * getInverterMaxPowerWatts() / 100),
                getInverterMaxPowerWatts());

        _spInverter->sendActivePowerControlRequest(newRelativeLimit,
                PowerLimitControlType::RelativNonPersistent);

        return true;
    };

    // disable power production as soon as possible.
    // setting the power limit is less important once the inverter is off.
    if (switchPowerState(false)) { return true; }

    if (updateLimit()) { return true; }

    // enable power production only after setting the desired limit
    if (switchPowerState(true)) { return true; }

    _updateTimeouts = 0;

    return reset();
}

std::optional<uint32_t> PowerLimiterInverter::getLatestStatsMillis() const
{
    uint32_t now = millis();

    // concerns both power limits and start/stop/restart commands and is
    // only updated if a respective response was received from the inverter
    auto lastUpdateCmdAge = std::min(
            now - _spInverter->SystemConfigPara()->getLastUpdateCommand(),
            now - _spInverter->PowerCommand()->getLastUpdateCommand()
    );

    // we use _oStatsMillis to persist a stats update timestamp, as we are
    // looking for the single oldest inverter stats which is still younger than
    // the last update command. we shall not just return the actual youngest
    // stats timestamp if newer stats arrived while no update command was sent
    // in the meantime.
    if (_oStatsMillis && lastUpdateCmdAge < (now - *_oStatsMillis)) {
        _oStatsMillis.reset();
    }

    if (!_oStatsMillis) {
        auto lastStatsMillis = _spInverter->Statistics()->getLastUpdate();
        auto lastStatsAge = now - lastStatsMillis;
        if (lastStatsAge > lastUpdateCmdAge) {
            return std::nullopt;
        }

        _oStatsMillis = lastStatsMillis;
    }

    return _oStatsMillis;
}

uint16_t PowerLimiterInverter::getInverterMaxPowerWatts() const
{
    return _spInverter->DevInfo()->getMaxPower();
}

uint16_t PowerLimiterInverter::getConfiguredMaxPowerWatts() const
{
    return std::min(getInverterMaxPowerWatts(), _config.UpperPowerLimit);
}

uint16_t PowerLimiterInverter::getCurrentOutputAcWatts() const
{
    return _spInverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC);
}

uint16_t PowerLimiterInverter::getExpectedOutputAcWatts() const
{
    if (!_oTargetPowerLimitWatts && !_oTargetPowerState) {
        // the inverter's output will not change due to commands being sent
        return getCurrentOutputAcWatts();
    }

    return _expectedOutputAcWatts;
}

void PowerLimiterInverter::setMaxOutput()
{
    _oTargetPowerState = true;
    setAcOutput(getConfiguredMaxPowerWatts());
}

void PowerLimiterInverter::restart()
{
    _spInverter->sendRestartControlRequest();
}

float PowerLimiterInverter::getDcVoltage(uint8_t input)
{
    return _spInverter->Statistics()->getChannelFieldValue(TYPE_DC,
            static_cast<ChannelNum_t>(input), FLD_UDC);
}

uint16_t PowerLimiterInverter::getCurrentLimitWatts() const
{
    auto currentLimitPercent = _spInverter->SystemConfigPara()->getLimitPercent();
    return static_cast<uint16_t>(currentLimitPercent * getInverterMaxPowerWatts() / 100);
}

void PowerLimiterInverter::debug() const
{
    if (!_verboseLogging) { return; }

    MessageOutput.printf(
        "%s\r\n"
        "    %s-powered, %s %d W\r\n"
        "    lower/current/upper limit: %d/%d/%d W, output capability: %d W\r\n"
        "    sending commands %s, %s, %s\r\n"
        "    max reduction production/standby: %d/%d W, max increase: %d W\r\n"
        "    target limit/output/state: %i W (%s)/%d W/%s, %d update timeouts\r\n",
        _logPrefix,
        (isSolarPowered()?"solar":"battery"),
        (isProducing()?"producing":"standing by at"), getCurrentOutputAcWatts(),
        _config.LowerPowerLimit, getCurrentLimitWatts(), _config.UpperPowerLimit,
        getInverterMaxPowerWatts(),
        (isSendingCommandsEnabled()?"enabled":"disabled"),
        (isReachable()?"reachable":"offline"),
        (isEligible()?"eligible":"disqualified"),
        getMaxReductionWatts(false), getMaxReductionWatts(true), getMaxIncreaseWatts(),
        (_oTargetPowerLimitWatts.has_value()?*_oTargetPowerLimitWatts:-1),
        (_oTargetPowerLimitWatts.has_value()?"update":"unchanged"),
        getExpectedOutputAcWatts(),
        (_oTargetPowerState.has_value()?(*_oTargetPowerState?"production":"standby"):"unchanged"),
        getUpdateTimeouts()
    );
}

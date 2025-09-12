#include "RestartHelper.h"
#include "PowerLimiterInverter.h"
#include "PowerLimiterBatteryInverter.h"
#include "PowerLimiterSolarInverter.h"
#include "PowerLimiterSmartBufferInverter.h"
#include "SunPosition.h"
#include <esp_log.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "dynamicPowerLimiter";
#define SUBTAG _logPrefix

std::unique_ptr<PowerLimiterInverter> PowerLimiterInverter::create(PowerLimiterInverterConfig const& config)
{
    std::unique_ptr<PowerLimiterInverter> upInverter;

    switch (config.PowerSource) {
        case PowerLimiterInverterConfig::InverterPowerSource::Battery:
            upInverter = std::make_unique<PowerLimiterBatteryInverter>(config);
            break;
        case PowerLimiterInverterConfig::InverterPowerSource::Solar:
            upInverter = std::make_unique<PowerLimiterSolarInverter>(config);
            break;
        case PowerLimiterInverterConfig::InverterPowerSource::SmartBuffer:
            upInverter = std::make_unique<PowerLimiterSmartBufferInverter>(config);
            break;
    }

    if (nullptr == upInverter->_spInverter) { return nullptr; }

    return std::move(upInverter);
}

PowerLimiterInverter::PowerLimiterInverter(PowerLimiterInverterConfig const& config)
    : _config(config)
{
    _spInverter = Hoymiles.getInverterBySerial(config.Serial);
    if (!_spInverter) { return; }

    snprintf(_serialStr, sizeof(_serialStr), "%0x%08x",
            static_cast<uint32_t>((config.Serial >> 32) & 0xFFFFFFFF),
            static_cast<uint32_t>(config.Serial & 0xFFFFFFFF));

    snprintf(_logPrefix, sizeof(_logPrefix), "Inverter %s", _serialStr);
}

PowerLimiterInverter::Eligibility PowerLimiterInverter::getEligibility() const
{
    // at dawn, solar-powered inverters switch to standby, but are still
    // reachable. during this time, we shall not use them. we assume that
    // it is already "night" when the inverter switches to standby, so this
    // check makes sense.
    if (isSolarPowered() && !SunPosition.isDayPeriod()) { return Eligibility::Nighttime; }

    if (!isReachable()) { return Eligibility::Unreachable; }

    if (!isSendingCommandsEnabled()) { return Eligibility::SendingCommandsDisabled; }

    // the model-dependent maximum AC power output is only known after the
    // first DevInfoSimpleCommand succeeded. we desperately need this info, so
    // the inverter is not eligible until this value is known.
    if (getInverterMaxPowerWatts() == 0) { return Eligibility::MaxOutputUnknown; }

    // after startup, the limit effective at the inverter is not known. the
    // respective message to request this info is only sent after a significant
    // backoff (~5 minutes, see upstream FAQ). this is to avoid error messages
    // to appear in the inverter's event log.
    if (getCurrentLimitWatts() == 0) { return Eligibility::CurrentLimitUnknown; }

    // inverters not connected to the grid are not eligible, as they cannot
    // produce power.
    if (getGridVoltage() < 100.0) { return Eligibility::GridDisconnected; }

    return Eligibility::Eligible;
}

bool PowerLimiterInverter::isEligible() const
{
    return getEligibility() == Eligibility::Eligible;
}

bool PowerLimiterInverter::update()
{
    auto reset = [this]() -> bool {
        _oTargetPowerState = std::nullopt;
        _oTargetPowerLimitWatts = std::nullopt;
        _oUpdateStartMillis = std::nullopt;
        return false;
    };

    switch (getEligibility()) {
        case Eligibility::Eligible:
            break;

        case Eligibility::CurrentLimitUnknown:
            // we actually can and must do something about this: set the configured
            // lower power limit. the inverter becomes eligible shortly and
            // inverters whose current limit is not fetched for some reason (see
            // #1427) are "woken up".
            if (!_oTargetPowerLimitWatts.has_value()) {
                DTU_LOGD("bootstrapping by setting lower power limit");
                _oTargetPowerLimitWatts = _config.LowerPowerLimit;
            }
            break;

        default:
            return reset();
            break;
    }

    // do not reset _updateTimeouts below if no state change requested
    if (!_oTargetPowerState.has_value() && !_oTargetPowerLimitWatts.has_value()) {
        return reset();
    }

    if (!_oUpdateStartMillis.has_value()) {
        _oUpdateStartMillis = millis();
    }

    auto updateFailure = [this,&reset]() -> bool {
        ++_updateTimeouts;

        // NOTE that these thresholds are not correlated to a specific time, since
        // this counts timeouts and failures, not absolute time. after any timeout or
        // failure, an update cycle ends. a new timeout or failure can only happen
        // after starting a new update cycle, which in turn is only started if the
        // DPL did calculate a new limit, which in turn does not happen while the
        // inverter is unreachable, no matter how long (a whole night) that might be.
        if (_updateTimeouts >= 20) {
            DTU_LOGE("restarting system since inverter is unresponsive");
            RestartHelper.triggerRestart();
        }
        else if (_updateTimeouts >= 10) {
            DTU_LOGW("issuing restart command after update timed out or failed %d times",
                    _updateTimeouts);
            _spInverter->sendRestartControlRequest();
        }

        return reset();
    };

    if ((millis() - *_oUpdateStartMillis) > 30 * 1000) {
        DTU_LOGW("timeout (%d in succession), state transition pending: %s, limit pending: %s",
                _updateTimeouts,
                (_oTargetPowerState.has_value()?"yes":"no"),
                (_oTargetPowerLimitWatts.has_value()?"yes":"no"));

        return updateFailure();
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
            DTU_LOGI("%s inverter...", ((*_oTargetPowerState)?"Starting":"Stopping"));
            _spInverter->sendPowerControlRequest(*_oTargetPowerState);
            return true;
        }

        _oTargetPowerState = std::nullopt; // target power state reached
        return false;
    };

    // we use a lambda function here to be able to use return statements,
    // which allows to avoid if-else-indentions and improves code readability
    auto updateLimit = [this,&updateFailure]() -> bool {
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
        // limit command completed and if it was sent after we started the last
        // update cycle, we should assume *our* requested limit was set.
        uint32_t lastLimitCommandMillis = _spInverter->SystemConfigPara()->getLastUpdateCommand();
        if ((lastLimitCommandMillis - *_oUpdateStartMillis) < halfOfAllMillis) {
            DTU_LOGD("limit update %s, actual limit is %.1f %% (%.0f W "
                    "respectively), effective %d ms after update started, "
                    "requested were %.1f %%",
                    (CMD_OK == lastLimitCommandState)?"succeeded":"FAILED",
                    currentRelativeLimit,
                    (currentRelativeLimit * getInverterMaxPowerWatts() / 100),
                    (lastLimitCommandMillis - *_oUpdateStartMillis),
                    newRelativeLimit);

            auto deviation = std::abs(newRelativeLimit - currentRelativeLimit);
            if (CMD_OK == lastLimitCommandState && deviation > 2.0) {
                DTU_LOGW("expected limit of %.1f %% and actual limit of "
                        "%.1f %% mismatch by more than 2 %%, is the DPL in exclusive "
                        "control over the inverter?",
                        newRelativeLimit, currentRelativeLimit);
            }

            _oTargetPowerLimitWatts = std::nullopt;

            if (CMD_OK != lastLimitCommandState) {
                // we don't retry a failed limit command, since it might as well
                // be outdated by now. the DPL will calculate a new limit for
                // the inverter and we will send that later instead.
                return updateFailure();
            }

            return false;
        }

        DTU_LOGI("sending limit of %.1f %% (%.0f W respectively), max output is %d W",
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

bool PowerLimiterInverter::retire()
{
    if (!_retired) { standby(); }
    _retired = true;
    return update();
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

float PowerLimiterInverter::getGridVoltage() const
{
    return _spInverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_UAC);
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
    if (!DTU_LOG_IS_VERBOSE) { return; }

    String eligibility("disqualified");
    switch (getEligibility()) {
        case Eligibility::Nighttime:
            eligibility += " (nighttime)";
            break;
        case Eligibility::Unreachable:
            eligibility += " (unreachable)";
            break;
        case Eligibility::SendingCommandsDisabled:
            eligibility += " (sending commands disabled)";
            break;
        case Eligibility::MaxOutputUnknown:
            eligibility += " (max output unknown)";
            break;
        case Eligibility::CurrentLimitUnknown:
            eligibility += " (current limit unknown)";
            break;
        case Eligibility::GridDisconnected:
            eligibility += " (grid disconnected)";
            break;
        case Eligibility::Eligible:
            eligibility = "eligible";
            break;
    }

    DTU_LOGV("State Details");
    DTU_LOGV("    %s-powered, %s %d W, output %s power meter reading",
        (isSmartBufferPowered()?"smart-buffer":(isSolarPowered()?"solar":"battery")),
        (isProducing()?"producing":"standing by at"), getCurrentOutputAcWatts(),
        (isBehindPowerMeter()?"included in":"excluded from")
    );
    DTU_LOGV("    lower/current/upper limit: %d/%d/%d W, output capability: %d W",
        _config.LowerPowerLimit, getCurrentLimitWatts(), _config.UpperPowerLimit,
        getInverterMaxPowerWatts()
    );
    DTU_LOGV("    sending commands %s, %s, %s",
        (isSendingCommandsEnabled()?"enabled":"disabled"),
        (isReachable()?"reachable":"offline"), eligibility.c_str()
    );
    DTU_LOGV("    max reduction production/standby: %d/%d W, max increase: %d W",
        getMaxReductionWatts(false), getMaxReductionWatts(true), getMaxIncreaseWatts()
    );
    DTU_LOGV("    target limit/output/state: %i W (%s)/%d W/%s, %d update timeouts",
        (_oTargetPowerLimitWatts.has_value()?*_oTargetPowerLimitWatts:-1),
        (_oTargetPowerLimitWatts.has_value()?"update":"unchanged"),
        getExpectedOutputAcWatts(),
        (_oTargetPowerState.has_value()?(*_oTargetPowerState?"production":"standby"):"unchanged"),
        getUpdateTimeouts()
    );

    char mpptDebug[160] = {0}; // AI says this is sufficient for up to 6 MPPTs
    size_t offset = snprintf(mpptDebug, sizeof(mpptDebug), "    MPPTs AC power/DC voltage:");

    auto pStats = _spInverter->Statistics();
    float inverterEfficiencyFactor = pStats->getChannelFieldValue(TYPE_INV, CH0, FLD_EFF) / 100;
    std::vector<MpptNum_t> dcMppts = _spInverter->getMppts();

    for (auto& m : dcMppts) {
        float mpptPowerAC = 0.0;
        float mpptVoltageDC = 0.0;
        std::vector<ChannelNum_t> mpptChnls = _spInverter->getChannelsDCByMppt(m);

        for (auto& c : mpptChnls) {
            mpptPowerAC += pStats->getChannelFieldValue(TYPE_DC, c, FLD_PDC) * inverterEfficiencyFactor;
            mpptVoltageDC = std::max(mpptVoltageDC, pStats->getChannelFieldValue(TYPE_DC, c, FLD_UDC));
        }

        offset += snprintf(mpptDebug + offset, sizeof(mpptDebug) - offset,
                " %c: %.0f W/%.1f V", mpptName(m), mpptPowerAC, mpptVoltageDC);
    }

    DTU_LOGV("%s", mpptDebug);
}

char PowerLimiterInverter::mpptName(MpptNum_t mppt)
{
    switch (mppt) {
        case MpptNum_t::MPPT_A:
            return 'a';

        case MpptNum_t::MPPT_B:
            return 'b';

        case MpptNum_t::MPPT_C:
            return 'c';

        case MpptNum_t::MPPT_D:
            return 'd';

        default:
            return '?';
    }
}

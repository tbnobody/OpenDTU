// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */

#include "Battery.h"
#include "PowerMeter.h"
#include "PowerLimiter.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Huawei_can.h"
#include <VictronMppt.h>
#include "MessageOutput.h"
#include "inverters/HMS_4CH.h"
#include <ctime>
#include <cmath>
#include <frozen/map.h>

PowerLimiterClass PowerLimiter;

void PowerLimiterClass::init(Scheduler& scheduler) 
{ 
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&PowerLimiterClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
}

frozen::string const& PowerLimiterClass::getStatusText(PowerLimiterClass::Status status)
{
    static const frozen::string missing = "programmer error: missing status text";

    static const frozen::map<Status, frozen::string, 18> texts = {
        { Status::Initializing, "initializing (should not see me)" },
        { Status::DisabledByConfig, "disabled by configuration" },
        { Status::DisabledByMqtt, "disabled by MQTT" },
        { Status::WaitingForValidTimestamp, "waiting for valid date and time to be available" },
        { Status::PowerMeterDisabled, "no power meter is configured/enabled" },
        { Status::PowerMeterTimeout, "power meter readings are outdated" },
        { Status::PowerMeterPending, "waiting for sufficiently recent power meter reading" },
        { Status::InverterInvalid, "invalid inverter selection/configuration" },
        { Status::InverterChanged, "target inverter changed" },
        { Status::InverterOffline, "inverter is offline (polling enabled? radio okay?)" },
        { Status::InverterCommandsDisabled, "inverter configuration prohibits sending commands" },
        { Status::InverterLimitPending, "waiting for a power limit command to complete" },
        { Status::InverterPowerCmdPending, "waiting for a start/stop/restart command to complete" },
        { Status::InverterDevInfoPending, "waiting for inverter device information to be available" },
        { Status::InverterStatsPending, "waiting for sufficiently recent inverter data" },
        { Status::UnconditionalSolarPassthrough, "unconditionally passing through all solar power (MQTT override)" },
        { Status::NoVeDirect, "VE.Direct disabled, connection broken, or data outdated" },
        { Status::Stable, "the system is stable, the last power limit is still valid" },
    };

    auto iter = texts.find(status);
    if (iter == texts.end()) { return missing; }

    return iter->second;
}

void PowerLimiterClass::announceStatus(PowerLimiterClass::Status status)
{
    // this method is called with high frequency. print the status text if
    // the status changed since we last printed the text of another one.
    // otherwise repeat the info with a fixed interval.
    if (_lastStatus == status && millis() < _lastStatusPrinted + 10 * 1000) { return; }

    // after announcing once that the DPL is disabled by configuration, it
    // should just be silent while it is disabled.
    if (status == Status::DisabledByConfig && _lastStatus == status) { return; }

    MessageOutput.printf("[%11.3f] DPL: %s\r\n",
        static_cast<double>(millis())/1000, getStatusText(status).data());

    _lastStatus = status;
    _lastStatusPrinted = millis();
}

/**
 * returns true if the inverter state was changed or is about to change, i.e.,
 * if it is actually in need of a shutdown. returns false otherwise, i.e., the
 * inverter is already shut down and the inverter limit is set to the configured
 * lower power limit.
 */
bool PowerLimiterClass::shutdown(PowerLimiterClass::Status status)
{
    announceStatus(status);

    _shutdownPending = true;

    _oTargetPowerState = false;
    _oTargetPowerLimitWatts = Configuration.get().PowerLimiter.LowerPowerLimit;
    return updateInverter();
}

void PowerLimiterClass::loop()
{
    CONFIG_T const& config = Configuration.get();
    _verboseLogging = config.PowerLimiter.VerboseLogging;

    // we know that the Hoymiles library refuses to send any message to any
    // inverter until the system has valid time information. until then we can
    // do nothing, not even shutdown the inverter.
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return announceStatus(Status::WaitingForValidTimestamp);
    }

    // take care that the last requested power
    // limit and power state are actually reached
    if (updateInverter()) { return; }

    if (_shutdownPending) {
        _shutdownPending = false;
        _inverter = nullptr;
    }

    if (!config.PowerLimiter.Enabled) {
        shutdown(Status::DisabledByConfig);
        return;
    }

    if (Mode::Disabled == _mode) {
        shutdown(Status::DisabledByMqtt);
        return;
    }

    std::shared_ptr<InverterAbstract> currentInverter =
        Hoymiles.getInverterByPos(config.PowerLimiter.InverterId);

    // in case of (newly) broken configuration, shut down
    // the last inverter we worked with (if any)
    if (currentInverter == nullptr) {
        shutdown(Status::InverterInvalid);
        return;
    }

    // if the DPL is supposed to manage another inverter now, we first
    // shut down the previous one, if any. then we pick up the new one.
    if (_inverter != nullptr && _inverter->serial() != currentInverter->serial()) {
        shutdown(Status::InverterChanged);
        return;
    }

    // update our pointer as the configuration might have changed
    _inverter = currentInverter;

    // data polling is disabled or the inverter is deemed offline
    if (!_inverter->isReachable()) {
        return announceStatus(Status::InverterOffline);
    }

    // sending commands to the inverter is disabled
    if (!_inverter->getEnableCommands()) {
        return announceStatus(Status::InverterCommandsDisabled);
    }

    // a calculated power limit will always be limited to the reported
    // device's max power. that upper limit is only known after the first
    // DevInfoSimpleCommand succeeded.
    if (_inverter->DevInfo()->getMaxPower() <= 0) {
        return announceStatus(Status::InverterDevInfoPending);
    }

    if (Mode::UnconditionalFullSolarPassthrough == _mode) {
        // handle this mode of operation separately
        return unconditionalSolarPassthrough(_inverter);
    }

    // the normal mode of operation requires a valid
    // power meter reading to calculate a power limit
    if (!config.PowerMeter.Enabled) {
        shutdown(Status::PowerMeterDisabled);
        return;
    }

    if (millis() - PowerMeter.getLastPowerMeterUpdate() > (30 * 1000)) {
        shutdown(Status::PowerMeterTimeout);
        return;
    }

    // concerns both power limits and start/stop/restart commands and is
    // only updated if a respective response was received from the inverter
    auto lastUpdateCmd = std::max(
            _inverter->SystemConfigPara()->getLastUpdateCommand(),
            _inverter->PowerCommand()->getLastUpdateCommand());

    if (_inverter->Statistics()->getLastUpdate() <= lastUpdateCmd) {
        return announceStatus(Status::InverterStatsPending);
    }

    if (PowerMeter.getLastPowerMeterUpdate() <= lastUpdateCmd) {
        return announceStatus(Status::PowerMeterPending);
    }

    // since _lastCalculation and _calculationBackoffMs are initialized to
    // zero, this test is passed the first time the condition is checked.
    if (millis() < (_lastCalculation + _calculationBackoffMs)) {
        return announceStatus(Status::Stable);
    }

    if (_verboseLogging) {
        MessageOutput.println("[DPL::loop] ******************* ENTER **********************");
    }

    // Check if next inverter restart time is reached
    if ((_nextInverterRestart > 1) && (_nextInverterRestart <= millis())) {
        MessageOutput.println("[DPL::loop] send inverter restart");
        _inverter->sendRestartControlRequest();
        calcNextInverterRestart();
    }

    // Check if NTP time is set and next inverter restart not calculated yet
    if ((config.PowerLimiter.RestartHour >= 0)  && (_nextInverterRestart == 0) ) {
        // check every 5 seconds
        if (_nextCalculateCheck < millis()) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 5)) {
                calcNextInverterRestart();
            } else {
                MessageOutput.println("[DPL::loop] inverter restart calculation: NTP not ready");
                _nextCalculateCheck += 5000;
            }
        }
    }

    // Battery charging cycle conditions
    // First we always disable discharge if the battery is empty
    if (isStopThresholdReached()) {
      // Disable battery discharge when empty
      _batteryDischargeEnabled = false;
    } else {
      // UI: Solar Passthrough Enabled -> false
      // Battery discharge can be enabled when start threshold is reached
      if (!config.PowerLimiter.SolarPassThroughEnabled && isStartThresholdReached()) {
        _batteryDischargeEnabled = true;
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_AT_NIGHT
      if (config.PowerLimiter.SolarPassThroughEnabled && config.PowerLimiter.BatteryDrainStategy == EMPTY_AT_NIGHT) {
        if(isStartThresholdReached()) {
            // In this case we should only discharge the battery as long it is above startThreshold
            _batteryDischargeEnabled = true;
        }
        else {
            // In this case we should only discharge the battery when there is no sunshine
            _batteryDischargeEnabled = !canUseDirectSolarPower();
        }
      }

      // UI: Solar Passthrough Enabled -> true && EMPTY_WHEN_FULL
      // Battery discharge can be enabled when start threshold is reached
      if (config.PowerLimiter.SolarPassThroughEnabled && isStartThresholdReached() && config.PowerLimiter.BatteryDrainStategy == EMPTY_WHEN_FULL) {
        _batteryDischargeEnabled = true;
      }
    }

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::loop] battery interface %s, SoC: %d %%, StartTH: %d %%, StopTH: %d %%, SoC age: %d s, ignore: %s\r\n",
                (config.Battery.Enabled?"enabled":"disabled"),
                Battery.getStats()->getSoC(),
                config.PowerLimiter.BatterySocStartThreshold,
                config.PowerLimiter.BatterySocStopThreshold,
                Battery.getStats()->getSoCAgeSeconds(),
                (config.PowerLimiter.IgnoreSoc?"yes":"no"));

        auto dcVoltage = getBatteryVoltage(true/*log voltages only once per DPL loop*/);
        MessageOutput.printf("[DPL::loop] dcVoltage: %.2f V, loadCorrectedVoltage: %.2f V, StartTH: %.2f V, StopTH: %.2f V\r\n",
                dcVoltage, getLoadCorrectedVoltage(),
                config.PowerLimiter.VoltageStartThreshold,
                config.PowerLimiter.VoltageStopThreshold);

        MessageOutput.printf("[DPL::loop] StartTH reached: %s, StopTH reached: %s, inverter %s producing\r\n",
                (isStartThresholdReached()?"yes":"no"),
                (isStopThresholdReached()?"yes":"no"),
                (_inverter->isProducing()?"is":"is NOT"));

        MessageOutput.printf("[DPL::loop] SolarPT %s, Drain Strategy: %i, canUseDirectSolarPower: %s\r\n",
                (config.PowerLimiter.SolarPassThroughEnabled?"enabled":"disabled"),
                config.PowerLimiter.BatteryDrainStategy, (canUseDirectSolarPower()?"yes":"no"));

        MessageOutput.printf("[DPL::loop] battery discharging %s, PowerMeter: %d W, target consumption: %d W\r\n",
                (_batteryDischargeEnabled?"allowed":"prevented"),
                static_cast<int32_t>(round(PowerMeter.getPowerTotal())),
                config.PowerLimiter.TargetPowerConsumption);
    }

    // Calculate and set Power Limit (NOTE: might reset _inverter to nullptr!)
    int32_t newPowerLimit = calcPowerLimit(_inverter, canUseDirectSolarPower(), _batteryDischargeEnabled);
    bool limitUpdated = setNewPowerLimit(_inverter, newPowerLimit);

    _lastCalculation = millis();

    if (!limitUpdated) {
        // increase polling backoff if system seems to be stable
        _calculationBackoffMs = std::min<uint32_t>(1024, _calculationBackoffMs * 2);
        return announceStatus(Status::Stable);
    }

    _calculationBackoffMs = _calculationBackoffMsDefault;
}

/**
 * determines the battery's voltage, trying multiple data providers. the most
 * accurate data is expected to be delivered by a BMS, if it's available. more
 * accurate and more recent than the inverter's voltage reading is the volage
 * at the charge controller's output, if it's available. only as a fallback
 * the voltage reported by the inverter is used.
 */
float PowerLimiterClass::getBatteryVoltage(bool log) {
    if (!_inverter) {
        // there should be no need to call this method if no target inverter is known
        MessageOutput.println("DPL getBatteryVoltage: no inverter (programmer error)");
        return 0.0;
    }

    auto const& config = Configuration.get();
    auto channel = static_cast<ChannelNum_t>(config.PowerLimiter.InverterChannelId);
    float inverterVoltage = _inverter->Statistics()->getChannelFieldValue(TYPE_DC, channel, FLD_UDC);
    float res = inverterVoltage;

    float chargeControllerVoltage = -1;
    if (VictronMppt.isDataValid()) {
        res = chargeControllerVoltage = static_cast<float>(VictronMppt.getOutputVoltage());
    }

    float bmsVoltage = -1;
    auto stats = Battery.getStats();
    if (config.Battery.Enabled
            && stats->isVoltageValid()
            && stats->getVoltageAgeSeconds() < 60) {
        res = bmsVoltage = stats->getVoltage();
    }

    if (log) {
        MessageOutput.printf("[DPL::getBatteryVoltage] BMS: %.2f V, MPPT: %.2f V, inverter: %.2f V, returning: %.2fV\r\n",
                bmsVoltage, chargeControllerVoltage, inverterVoltage, res);
    }

    return res;
}

/**
 * calculate the AC output power (limit) to set, such that the inverter uses
 * the given power on its DC side, i.e., adjust the power for the inverter's
 * efficiency.
 */
int32_t PowerLimiterClass::inverterPowerDcToAc(std::shared_ptr<InverterAbstract> inverter, int32_t dcPower)
{
    CONFIG_T& config = Configuration.get();

    float inverterEfficiencyPercent = inverter->Statistics()->getChannelFieldValue(
        TYPE_AC, CH0, FLD_EFF);

    // fall back to hoymiles peak efficiency as per datasheet if inverter
    // is currently not producing (efficiency is zero in that case)
    float inverterEfficiencyFactor = (inverterEfficiencyPercent > 0) ? inverterEfficiencyPercent/100 : 0.967;

    // account for losses between solar charger and inverter (cables, junctions...)
    float lossesFactor = 1.00 - static_cast<float>(config.PowerLimiter.SolarPassThroughLosses)/100;

    return dcPower * inverterEfficiencyFactor * lossesFactor;
}

/**
 * implements the "unconditional solar passthrough" mode of operation, which
 * can currently only be set using MQTT. in this mode of operation, the
 * inverter shall behave as if it was connected to the solar panels directly,
 * i.e., all solar power (and only solar power) is fed to the AC side,
 * independent from the power meter reading.
 */
void PowerLimiterClass::unconditionalSolarPassthrough(std::shared_ptr<InverterAbstract> inverter)
{
    if (!VictronMppt.isDataValid()) {
        shutdown(Status::NoVeDirect);
        return;
    }

    int32_t solarPower = VictronMppt.getPowerOutputWatts();
    setNewPowerLimit(inverter, inverterPowerDcToAc(inverter, solarPower));
    announceStatus(Status::UnconditionalSolarPassthrough);
}

uint8_t PowerLimiterClass::getPowerLimiterState() {
    if (_inverter == nullptr || !_inverter->isReachable()) {
        return PL_UI_STATE_INACTIVE;
    }

    if (_inverter->isProducing() && _batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_AND_BATTERY;
    }

    if (_inverter->isProducing() && !_batteryDischargeEnabled) {
      return PL_UI_STATE_USE_SOLAR_ONLY;
    }

    if(!_inverter->isProducing()) {
       return PL_UI_STATE_CHARGING;
    }

    return PL_UI_STATE_INACTIVE;
}

bool PowerLimiterClass::canUseDirectSolarPower()
{
    CONFIG_T& config = Configuration.get();

    if (!config.PowerLimiter.SolarPassThroughEnabled
            || isBelowStopThreshold()
            || !VictronMppt.isDataValid()) {
        return false;
    }

    return VictronMppt.getPowerOutputWatts() >= 20; // enough power?
}


// Logic table
// | Case # | batteryDischargeEnabled | solarPowerEnabled | useFullSolarPassthrough | Result                                                      |
// | 1      | false                   | false             | doesn't matter          | PL = 0                                                      |
// | 2      | false                   | true              | doesn't matter          | PL = Victron Power                                          |
// | 3      | true                    | doesn't matter    | false                   | PL = PowerMeter value (Battery can supply unlimited energy) |
// | 4      | true                    | false             | true                    | PL = PowerMeter value                                       |
// | 5      | true                    | true              | true                    | PL = max(PowerMeter value, Victron Power)                   |

int32_t PowerLimiterClass::calcPowerLimit(std::shared_ptr<InverterAbstract> inverter, bool solarPowerEnabled, bool batteryDischargeEnabled)
{
    CONFIG_T& config = Configuration.get();
    
    int32_t acPower = 0;
    int32_t newPowerLimit = round(PowerMeter.getPowerTotal());

    if (!solarPowerEnabled && !batteryDischargeEnabled) {
      // Case 1 - No energy sources available
      return 0;
    }

    if (config.PowerLimiter.IsInverterBehindPowerMeter) {
        // If the inverter the behind the power meter (part of measurement),
        // the produced power of this inverter has also to be taken into account.
        // We don't use FLD_PAC from the statistics, because that
        // data might be too old and unreliable.
        acPower = static_cast<int>(inverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC)); 
        newPowerLimit += acPower;
    }

    // We're not trying to hit 0 exactly but take an offset into account
    // This means we never fully compensate the used power with the inverter 
    // Case 3
    newPowerLimit -= config.PowerLimiter.TargetPowerConsumption;

    // At this point we've calculated the required energy to compensate for household consumption. 
    // If the battery is enabled this can always be supplied since we assume that the battery can supply unlimited power
    // The next step is to determine if the Solar power as provided by the Victron charger
    // actually constrains or dictates another inverter power value
    int32_t adjustedVictronChargePower = inverterPowerDcToAc(inverter, getSolarChargePower());

    // Battery can be discharged and we should output max (Victron solar power || power meter value)
    if(batteryDischargeEnabled && useFullSolarPassthrough()) {
      // Case 5
      newPowerLimit = newPowerLimit > adjustedVictronChargePower ? newPowerLimit : adjustedVictronChargePower;
    } else {
      // We check if the PSU is on and disable the Power Limiter in this case. 
      // The PSU should reduce power or shut down first before the Power Limiter kicks in
      // The only case where this is not desired is if the battery is over the Full Solar Passthrough Threshold
      // In this case the Power Limiter should start. The PSU will shutdown when the Power Limiter is active
      if (HuaweiCan.getAutoPowerStatus()) {
        return 0;
      }
    }

    // We should use Victron solar power only (corrected by efficiency factor)
    if (solarPowerEnabled && !batteryDischargeEnabled) {
        // Case 2 - Limit power to solar power only
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::loop] Consuming Solar Power Only -> adjustedVictronChargePower: %d W, newPowerLimit: %d W\r\n",
                adjustedVictronChargePower, newPowerLimit);
        }

        newPowerLimit = std::min(newPowerLimit, adjustedVictronChargePower);
    }

    return newPowerLimit;
}

/**
 * updates the inverter state (power production and limit). returns true if a
 * change to its state was requested or is pending. this function only requests
 * one change (limit value or production on/off) at a time.
 */
bool PowerLimiterClass::updateInverter()
{
    auto reset = [this]() -> bool {
        _oTargetPowerState = std::nullopt;
        _oTargetPowerLimitWatts = std::nullopt;
        _oUpdateStartMillis = std::nullopt;
        return false;
    };

    if (nullptr == _inverter) { return reset(); }

    if (!_oUpdateStartMillis.has_value()) {
        _oUpdateStartMillis = millis();
    }

    if ((millis() - *_oUpdateStartMillis) > 30 * 1000) {
        MessageOutput.printf("[DPL::updateInverter] timeout, "
                "state transition pending: %s, limit pending: %s\r\n",
                (_oTargetPowerState.has_value()?"yes":"no"),
                (_oTargetPowerLimitWatts.has_value()?"yes":"no"));
        return reset();
    }

    auto constexpr halfOfAllMillis = std::numeric_limits<uint32_t>::max() / 2;

    auto switchPowerState = [this](bool transitionOn) -> bool {
        // no power state transition requested at all
        if (!_oTargetPowerState.has_value()) { return false; }

        // the transition that may be started is not the one which is requested
        if (transitionOn != *_oTargetPowerState) { return false; }

        // wait for pending power command(s) to complete
        auto lastPowerCommandState = _inverter->PowerCommand()->getLastPowerCommandSuccess();
        if (CMD_PENDING == lastPowerCommandState) {
            announceStatus(Status::InverterPowerCmdPending);
            return true;
        }

        // we need to wait for statistics that are more recent than the last
        // power update command to reliably use _inverter->isProducing()
        auto lastPowerCommandMillis = _inverter->PowerCommand()->getLastUpdateCommand();
        auto lastStatisticsMillis = _inverter->Statistics()->getLastUpdate();
        if ((lastStatisticsMillis - lastPowerCommandMillis) > halfOfAllMillis) { return true; }

        if (_inverter->isProducing() != *_oTargetPowerState) {
            MessageOutput.printf("[DPL::updateInverter] %s inverter...\r\n",
                    ((*_oTargetPowerState)?"Starting":"Stopping"));
            _inverter->sendPowerControlRequest(*_oTargetPowerState);
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
        auto lastLimitCommandState = _inverter->SystemConfigPara()->getLastLimitCommandSuccess();
        if (CMD_PENDING == lastLimitCommandState) {
            announceStatus(Status::InverterLimitPending);
            return true;
        }

        auto maxPower = _inverter->DevInfo()->getMaxPower();
        auto newRelativeLimit = static_cast<float>(*_oTargetPowerLimitWatts * 100) / maxPower;

        // if no limit command is pending, the SystemConfigPara does report the
        // current limit, as the answer by the inverter to a limit command is
        // the canonical source that updates the known current limit.
        auto currentRelativeLimit = _inverter->SystemConfigPara()->getLimitPercent();

        // we assume having exclusive control over the inverter. if the last
        // limit command was successful and sent after we started the last
        // update cycle, we should assume *our* requested limit was set.
        uint32_t lastLimitCommandMillis = _inverter->SystemConfigPara()->getLastUpdateCommand();
        if ((lastLimitCommandMillis - *_oUpdateStartMillis) < halfOfAllMillis &&
                CMD_OK == lastLimitCommandState) {
            MessageOutput.printf("[DPL:updateInverter] actual limit is %.1f %% "
                    "(%.0f W respectively), effective %d ms after update started, "
                    "requested were %.1f %%\r\n",
                    currentRelativeLimit,
                    (currentRelativeLimit * maxPower / 100),
                    (lastLimitCommandMillis - *_oUpdateStartMillis),
                    newRelativeLimit);

            if (std::abs(newRelativeLimit - currentRelativeLimit) > 2.0) {
                MessageOutput.printf("[DPL:updateInverter] NOTE: expected limit of %.1f %% "
                        "and actual limit of %.1f %% mismatch by more than 2 %%, "
                        "is the DPL in exclusive control over the inverter?\r\n",
                        newRelativeLimit, currentRelativeLimit);
            }

            _oTargetPowerLimitWatts = std::nullopt;
            return false;
        }

        MessageOutput.printf("[DPL::updateInverter] sending limit of %.1f %% "
                "(%.0f W respectively), max output is %d W\r\n",
                newRelativeLimit, (newRelativeLimit * maxPower / 100), maxPower);

        _inverter->sendActivePowerControlRequest(static_cast<float>(newRelativeLimit),
                PowerLimitControlType::RelativNonPersistent);

        _lastRequestedPowerLimit = *_oTargetPowerLimitWatts;
        return true;
    };

    // disable power production as soon as possible.
    // setting the power limit is less important once the inverter is off.
    if (switchPowerState(false)) { return true; }

    if (updateLimit()) { return true; }

    // enable power production only after setting the desired limit
    if (switchPowerState(true)) { return true; }

    return reset();
}

/**
 * scale the desired inverter limit such that the actual inverter AC output is
 * close to the desired power limit, even if some input channels are producing
 * less than the limit allows. this happens because the inverter seems to split
 * the total power limit equally among all MPPTs (not inputs; some inputs share
 * the same MPPT on some models).
 *
 * TODO(schlimmchen): the current implementation is broken and is in need of
 * refactoring. currently it only works for inverters that provide one MPPT for
 * each input. it also does not work as expected if any input produces *some*
 * energy, but is limited by its respective solar input.
 */
static int32_t scalePowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newLimit, int32_t currentLimitWatts)
{
    // prevent scaling if inverter is not producing, as input channels are not
    // producing energy and hence are detected as not-producing, causing
    // unreasonable scaling.
    if (!inverter->isProducing()) { return newLimit; }

    std::list<ChannelNum_t> dcChnls = inverter->Statistics()->getChannelsByType(TYPE_DC);
    size_t dcTotalChnls = dcChnls.size();

    // according to the upstream projects README (table with supported devs),
    // every 2 channel inverter has 2 MPPTs. then there are the HM*S* 4 channel
    // models which have 4 MPPTs. all others have a different number of MPPTs
    // than inputs. those are not supported by the current scaling mechanism.
    bool supported = dcTotalChnls == 2;
    supported |= dcTotalChnls == 4 && HMS_4CH::isValidSerial(inverter->serial());
    if (!supported) { return newLimit; }

    // test for a reasonable power limit that allows us to assume that an input
    // channel with little energy is actually not producing, rather than
    // producing very little due to the very low limit.
    if (currentLimitWatts < dcTotalChnls * 10) { return newLimit; }

    size_t dcProdChnls = 0;
    for (auto& c : dcChnls) {
        if (inverter->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC) > 2.0) {
            dcProdChnls++;
        }
    }

    if (dcProdChnls == dcTotalChnls) { return newLimit; }

    MessageOutput.printf("[DPL::scalePowerLimit] %d channels total, %d producing "
            "channels, scaling power limit\r\n", dcTotalChnls, dcProdChnls);
    return round(newLimit * static_cast<float>(dcTotalChnls) / dcProdChnls);
}

/**
 * enforces limits on the requested power limit, after scaling the power limit
 * to the ratio of total and producing inverter channels. commits the sanitized
 * power limit. returns true if an inverter update was committed, false
 * otherwise.
 */
bool PowerLimiterClass::setNewPowerLimit(std::shared_ptr<InverterAbstract> inverter, int32_t newPowerLimit)
{
    CONFIG_T& config = Configuration.get();

    // Stop the inverter if limit is below threshold.
    if (newPowerLimit < config.PowerLimiter.LowerPowerLimit) {
        // the status must not change outside of loop(). this condition is
        // communicated through log messages already.
        return shutdown();
    }

    // enforce configured upper power limit
    int32_t effPowerLimit = std::min(newPowerLimit, config.PowerLimiter.UpperPowerLimit);

    // early in the loop we make it a pre-requisite that this
    // value is non-zero, so we can assume it to be valid.
    auto maxPower = inverter->DevInfo()->getMaxPower();

    float currentLimitPercent = inverter->SystemConfigPara()->getLimitPercent();
    auto currentLimitAbs = static_cast<int32_t>(currentLimitPercent * maxPower / 100);

    effPowerLimit = scalePowerLimit(inverter, effPowerLimit, currentLimitAbs);

    effPowerLimit = std::min<int32_t>(effPowerLimit, maxPower);

    auto diff = std::abs(currentLimitAbs - effPowerLimit);
    auto hysteresis = config.PowerLimiter.TargetPowerConsumptionHysteresis;

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::setNewPowerLimit] calculated: %d W, "
                "requesting: %d W, reported: %d W, diff: %d W, hysteresis: %d W\r\n",
                newPowerLimit, effPowerLimit, currentLimitAbs, diff, hysteresis);
    }

    if (diff > hysteresis) {
        _oTargetPowerLimitWatts = effPowerLimit;
    }

    _oTargetPowerState = true;
    return updateInverter();
}

int32_t PowerLimiterClass::getSolarChargePower()
{
    if (!canUseDirectSolarPower()) {
        return 0;
    }

    return VictronMppt.getPowerOutputWatts();
}

float PowerLimiterClass::getLoadCorrectedVoltage()
{
    if (!_inverter) {
        // there should be no need to call this method if no target inverter is known
        MessageOutput.println("DPL getLoadCorrectedVoltage: no inverter (programmer error)");
        return 0.0;
    }

    CONFIG_T& config = Configuration.get();

    float acPower = _inverter->Statistics()->getChannelFieldValue(TYPE_AC, CH0, FLD_PAC);
    float dcVoltage = getBatteryVoltage();

    if (dcVoltage <= 0.0) {
        return 0.0;
    }

    return dcVoltage + (acPower * config.PowerLimiter.VoltageLoadCorrectionFactor);
}

bool PowerLimiterClass::testThreshold(float socThreshold, float voltThreshold,
        std::function<bool(float, float)> compare)
{
    CONFIG_T& config = Configuration.get();

    // prefer SoC provided through battery interface, unless disabled by user
    auto stats = Battery.getStats();
    if (!config.PowerLimiter.IgnoreSoc
            && config.Battery.Enabled
            && socThreshold > 0.0
            && stats->isSoCValid()
            && stats->getSoCAgeSeconds() < 60) {
              return compare(stats->getSoC(), socThreshold);
    }

    // use voltage threshold as fallback
    if (voltThreshold <= 0.0) { return false; }

    return compare(getLoadCorrectedVoltage(), voltThreshold);
}

bool PowerLimiterClass::isStartThresholdReached()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStartThreshold,
            config.PowerLimiter.VoltageStartThreshold,
            [](float a, float b) -> bool { return a >= b; }
    );
}

bool PowerLimiterClass::isStopThresholdReached()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStopThreshold,
            config.PowerLimiter.VoltageStopThreshold,
            [](float a, float b) -> bool { return a <= b; }
    );
}

bool PowerLimiterClass::isBelowStopThreshold()
{
    CONFIG_T& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStopThreshold,
            config.PowerLimiter.VoltageStopThreshold,
            [](float a, float b) -> bool { return a < b; }
    );
}

/// @brief calculate next inverter restart in millis
void PowerLimiterClass::calcNextInverterRestart()
{
    CONFIG_T& config = Configuration.get();

    // first check if restart is configured at all
    if (config.PowerLimiter.RestartHour < 0) {
        _nextInverterRestart = 1;
        MessageOutput.println("[DPL::calcNextInverterRestart] _nextInverterRestart disabled");
        return;
    }

    // read time from timeserver, if time is not synced then return
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5)) {
        // calculation first step is offset to next restart in minutes
        uint16_t dayMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
        uint16_t targetMinutes = config.PowerLimiter.RestartHour * 60;
        if (config.PowerLimiter.RestartHour > timeinfo.tm_hour) {
            // next restart is on the same day
            _nextInverterRestart = targetMinutes - dayMinutes;
        } else {
            // next restart is on next day
            _nextInverterRestart = 1440 - dayMinutes + targetMinutes;
        }
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::calcNextInverterRestart] Localtime read %d %d / configured RestartHour %d\r\n", timeinfo.tm_hour, timeinfo.tm_min, config.PowerLimiter.RestartHour);
            MessageOutput.printf("[DPL::calcNextInverterRestart] dayMinutes %d / targetMinutes %d\r\n", dayMinutes, targetMinutes);
            MessageOutput.printf("[DPL::calcNextInverterRestart] next inverter restart in %d minutes\r\n", _nextInverterRestart);
        }
        // then convert unit for next restart to milliseconds and add current uptime millis()
        _nextInverterRestart *= 60000;
        _nextInverterRestart += millis();
    } else {
        MessageOutput.println("[DPL::calcNextInverterRestart] getLocalTime not successful, no calculation");
        _nextInverterRestart = 0;
    }
    MessageOutput.printf("[DPL::calcNextInverterRestart] _nextInverterRestart @ %d millis\r\n", _nextInverterRestart);
}

bool PowerLimiterClass::useFullSolarPassthrough()
{
    CONFIG_T& config = Configuration.get();

    // We only do full solar PT if general solar PT is enabled
    if(!config.PowerLimiter.SolarPassThroughEnabled) {
      return false;
    }

    if (testThreshold(config.PowerLimiter.FullSolarPassThroughSoc,
                      config.PowerLimiter.FullSolarPassThroughStartVoltage,
                      [](float a, float b) -> bool { return a >= b; })) {
        _fullSolarPassThroughEnabled = true;
    }

    if (testThreshold(config.PowerLimiter.FullSolarPassThroughSoc,
                      config.PowerLimiter.FullSolarPassThroughStopVoltage,
                      [](float a, float b) -> bool { return a < b; })) {
        _fullSolarPassThroughEnabled = false;
    }

    return _fullSolarPassThroughEnabled;
}

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
#include <ctime>
#include <cmath>
#include <limits>
#include <frozen/map.h>
#include "SunPosition.h"

static auto sBatteryPoweredFilter = [](PowerLimiterInverter const& inv) {
    return !inv.isSolarPowered();
};

static const char sBatteryPoweredExpression[] = "battery-powered inverters";

static auto sSolarPoweredFilter = [](PowerLimiterInverter const& inv) {
    return inv.isSolarPowered();
};

static const char sSolarPoweredExpression[] = "solar-powered inverters";

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

    static const frozen::map<Status, frozen::string, 12> texts = {
        { Status::Initializing, "initializing (should not see me)" },
        { Status::DisabledByConfig, "disabled by configuration" },
        { Status::DisabledByMqtt, "disabled by MQTT" },
        { Status::WaitingForValidTimestamp, "waiting for valid date and time to be available" },
        { Status::PowerMeterPending, "waiting for sufficiently recent power meter reading" },
        { Status::InverterInvalid, "invalid inverter selection/configuration" },
        { Status::InverterCmdPending, "waiting for a start/stop/restart/limit command to complete" },
        { Status::ConfigReload, "reloading DPL configuration" },
        { Status::InverterStatsPending, "waiting for sufficiently recent inverter data" },
        { Status::FullSolarPassthrough, "passing through all solar power (full solar passthrough)" },
        { Status::UnconditionalSolarPassthrough, "unconditionally passing through all solar power (MQTT override)" },
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

    MessageOutput.printf("[DPL::announceStatus] %s\r\n",
        getStatusText(status).data());

    _lastStatus = status;
    _lastStatusPrinted = millis();
}

/**
 * returns true if the inverters' state was changed or is about to change, i.e.,
 * if any are actually in need of a shutdown. returns false otherwise, i.e., the
 * inverters are already shut down.
 */
bool PowerLimiterClass::shutdown(PowerLimiterClass::Status status)
{
    announceStatus(status);

    for (auto& upInv : _inverters) { upInv->standby(); }

    return updateInverters();
}

void PowerLimiterClass::reloadConfig()
{
    auto const& config = Configuration.get();

    _verboseLogging = config.PowerLimiter.VerboseLogging;

    // clean up all inverter instances. put inverters into
    // standby if they will not be governed any more.
    auto iter = _inverters.begin();
    while (iter != _inverters.end()) {
        bool stillGoverned = false;

        for (size_t i = 0; i < INV_MAX_COUNT; ++i) {
            auto const& inv = config.PowerLimiter.Inverters[i];
            if (inv.Serial == 0ULL) { break; }
            stillGoverned = inv.Serial == (*iter)->getSerial() && inv.IsGoverned;
            if (stillGoverned) { break; }
        }

        if (!stillGoverned) {
            (*iter)->standby();
            if ((*iter)->update()) { return; }
        }

        iter = _inverters.erase(iter);
    }

    for (size_t i = 0; i < INV_MAX_COUNT; ++i) {
        auto const& invConfig = config.PowerLimiter.Inverters[i];

        if (invConfig.Serial == 0ULL) { break; }

        if (!invConfig.IsGoverned) { continue; }

        auto upInv = PowerLimiterInverter::create(_verboseLogging, invConfig);
        if (upInv) { _inverters.push_back(std::move(upInv)); }
    }

    _reloadConfigFlag = false;
}

void PowerLimiterClass::loop()
{
    auto const& config = Configuration.get();

    // we know that the Hoymiles library refuses to send any message to any
    // inverter until the system has valid time information. until then we can
    // do nothing, not even shutdown the inverter.
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        return announceStatus(Status::WaitingForValidTimestamp);
    }

    // take care that the last requested power
    // limits and power states are actually reached
    if (updateInverters()) {
        return announceStatus(Status::InverterCmdPending);
    }

    if (!config.PowerLimiter.Enabled) {
        shutdown(Status::DisabledByConfig);
        return;
    }

    if (Mode::Disabled == _mode) {
        shutdown(Status::DisabledByMqtt);
        return;
    }

    if (_reloadConfigFlag) {
        reloadConfig();
        return announceStatus(Status::ConfigReload);
    }

    if (_inverters.empty()) {
        return announceStatus(Status::InverterInvalid);
    }

    if (Mode::UnconditionalFullSolarPassthrough == _mode) {
        return fullSolarPassthrough(Status::UnconditionalSolarPassthrough);
    }

    if (isFullSolarPassthroughActive()) {
        return fullSolarPassthrough(Status::FullSolarPassthrough);
    }

    uint32_t latestInverterStats = 0;

    for (auto const& upInv : _inverters) {
        auto oStatsMillis = upInv->getLatestStatsMillis();
        if (!oStatsMillis) {
            return announceStatus(Status::InverterStatsPending);
        }

        latestInverterStats = std::max(*oStatsMillis, latestInverterStats);
    }

    // if the power meter is being used, i.e., if its data is valid, we want to
    // wait for a new reading after adjusting the inverter limit. otherwise, we
    // proceed as we will use a fallback limit independent of the power meter.
    // the power meter reading is expected to be at most 2 seconds old when it
    // arrives. this can be the case for readings provided by networked meter
    // readers, where a packet needs to travel through the network for some
    // time after the actual measurement was done by the reader.
    if (PowerMeter.isDataValid() && PowerMeter.getLastUpdate() <= (latestInverterStats + 2000)) {
        return announceStatus(Status::PowerMeterPending);
    }

    // since _lastCalculation and _calculationBackoffMs are initialized to
    // zero, this test is passed the first time the condition is checked.
    if ((millis() - _lastCalculation) < _calculationBackoffMs) {
        return announceStatus(Status::Stable);
    }

    if (_verboseLogging) {
        MessageOutput.println("[DPL::loop] ******************* ENTER **********************");
    }

    // TODO(schlimmchen): comparison breaks when millis() wraps around.
    // Check if next inverter restart time is reached
    if ((_nextInverterRestart > 1) && (_nextInverterRestart <= millis())) {
        MessageOutput.println("[DPL::loop] send inverter restart");
        for (auto& upInv : _inverters) {
            if (!upInv->isSolarPowered()) { upInv->restart(); }
        }
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

    auto getBatteryPower = [this,&config]() -> bool {
        if (!usesBatteryPoweredInverter()) { return false; }

        auto isDayPeriod = SunPosition.isSunsetAvailable() ? SunPosition.isDayPeriod() : true;

        if (_nighttimeDischarging && isDayPeriod) {
            _nighttimeDischarging = false;
            return isStartThresholdReached();
        }

        if (isStopThresholdReached()) { return false; }

        if (isStartThresholdReached()) { return true; }

        if (config.PowerLimiter.BatteryAlwaysUseAtNight &&
                !isDayPeriod &&
                !_batteryDischargeEnabled) {
            _nighttimeDischarging = true;
            return true;
        }

        // we are between start and stop threshold and keep the state that was
        // last triggered, either charging or discharging.
        return _batteryDischargeEnabled;
    };

    _batteryDischargeEnabled = getBatteryPower();

    if (_verboseLogging && usesBatteryPoweredInverter()) {
        MessageOutput.printf("[DPL::loop] battery interface %s, SoC: %.1f %%, StartTH: %d %%, StopTH: %d %%, SoC age: %d s, ignore: %s\r\n",
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

        MessageOutput.printf("[DPL::loop] StartTH reached: %s, StopTH reached: %s, SolarPT %sabled, use at night: %s\r\n",
                (isStartThresholdReached()?"yes":"no"),
                (isStopThresholdReached()?"yes":"no"),
                (config.PowerLimiter.SolarPassThroughEnabled?"en":"dis"),
                (config.PowerLimiter.BatteryAlwaysUseAtNight?"yes":"no"));
    };

    // this value is negative if we are exporting power to the grid
    // from power sources other than DPL-governed inverters.
    int16_t consumption = calcHouseholdConsumption();

    uint16_t inverterTotalPower = (consumption > 0) ? static_cast<uint16_t>(consumption) : 0;

    auto totalAllowance = config.PowerLimiter.TotalUpperPowerLimit;
    inverterTotalPower = std::min(inverterTotalPower, totalAllowance);

    auto coveredBySolar = updateInverterLimits(inverterTotalPower, sSolarPoweredFilter, sSolarPoweredExpression);
    auto remaining = (inverterTotalPower >= coveredBySolar) ? inverterTotalPower - coveredBySolar : 0;
    auto batteryAllowance = calcBatteryAllowance(remaining);
    auto coveredByBattery = updateInverterLimits(batteryAllowance, sBatteryPoweredFilter, sBatteryPoweredExpression);

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::loop] consumption: %d W, "
                "target output: %u W (limited to %d W), "
                "solar inverters output: %u W, battery allowance: "
                "%u W, battery inverters output: %u W\r\n",
                consumption, inverterTotalPower, totalAllowance,
                coveredBySolar, batteryAllowance, coveredByBattery);
    }

    _lastExpectedInverterOutput = coveredBySolar + coveredByBattery;

    bool limitUpdated = updateInverters();

    _lastCalculation = millis();

    if (!limitUpdated) {
        // increase polling backoff if system seems to be stable
        _calculationBackoffMs = std::min<uint32_t>(1024, _calculationBackoffMs * 2);
        return announceStatus(Status::Stable);
    }

    _calculationBackoffMs = _calculationBackoffMsDefault;
}

std::pair<float, char const*> PowerLimiterClass::getInverterDcVoltage() {
    auto const& config = Configuration.get();

    auto iter = _inverters.begin();
    while(iter != _inverters.end()) {
        if ((*iter)->getSerial() == config.PowerLimiter.InverterSerialForDcVoltage) {
            break;
        }
        ++iter;
    }

    if (iter == _inverters.end()) {
        return { -1.0, "<unknown>" };
    }

    auto voltage = (*iter)->getDcVoltage(config.PowerLimiter.InverterChannelIdForDcVoltage);
    return { voltage, (*iter)->getSerialStr() };
}

/**
 * determines the battery's voltage, trying multiple data providers. the most
 * accurate data is expected to be delivered by a BMS, if it's available. more
 * accurate and more recent than the inverter's voltage reading is the volage
 * at the charge controller's output, if it's available. only as a fallback
 * the voltage reported by the inverter is used.
 */
float PowerLimiterClass::getBatteryVoltage(bool log) {
    auto const& config = Configuration.get();

    float res = 0;

    auto inverter = getInverterDcVoltage();
    if (inverter.first > 0) { res = inverter.first; }

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
        MessageOutput.printf("[DPL::getBatteryVoltage] BMS: %.2f V, MPPT: %.2f V, "
                "inverter %s: %.2f V, returning: %.2fV\r\n", bmsVoltage,
                chargeControllerVoltage, inverter.second, inverter.first, res);
    }

    return res;
}

/**
 * calculate the AC output power (limit) to set, such that the inverter uses
 * the given power on its DC side, i.e., adjust the power for the inverter's
 * efficiency.
 */
uint16_t PowerLimiterClass::solarDcToInverterAc(uint16_t dcPower)
{
    // account for losses between solar charger and inverter (cables, junctions...)
    auto const& config = Configuration.get();
    float lossesFactor = 1.00 - static_cast<float>(config.PowerLimiter.SolarPassThroughLosses)/100;

    // we cannot know the efficiency at the new limit. even if we could we
    // cannot know which inverter is assigned which limit. hence we use a
    // reasonable, conservative, fixed inverter efficiency.
    return 0.95 * lossesFactor * dcPower;
}

/**
 * implements the "full solar passthrough" mode of operation. in this mode of
 * operation, the inverters shall behave as if they were connected to the solar
 * panels directly, i.e., all solar power (and only solar power) is converted
 * to AC power, independent from the power meter reading.
 */
void PowerLimiterClass::fullSolarPassthrough(PowerLimiterClass::Status reason)
{
    if ((millis() - _lastCalculation) < _calculationBackoffMs) { return; }
    _lastCalculation = millis();

    for (auto& upInv : _inverters) {
        if (upInv->isSolarPowered()) { upInv->setMaxOutput(); }
    }

    uint16_t targetOutput = 0;

    if (VictronMppt.isDataValid()) {
        targetOutput = static_cast<uint16_t>(std::max<int32_t>(0, VictronMppt.getPowerOutputWatts()));
        targetOutput = solarDcToInverterAc(targetOutput);
    }

    _calculationBackoffMs = 1 * 1000;
    updateInverterLimits(targetOutput, sBatteryPoweredFilter, sBatteryPoweredExpression);
    return announceStatus(reason);
}

uint8_t PowerLimiterClass::getInverterUpdateTimeouts() const
{
    uint8_t res = 0;
    for (auto const& upInv : _inverters) {
        res += upInv->getUpdateTimeouts();
    }
    return res;
}

uint8_t PowerLimiterClass::getPowerLimiterState()
{
    bool reachable = false;
    bool producing = false;
    for (auto const& upInv : _inverters) {
        reachable |= upInv->isReachable();
        producing |= upInv->isProducing();
    }

    if (!reachable) {
        return PL_UI_STATE_INACTIVE;
    }

    if (!producing) {
        return PL_UI_STATE_CHARGING;
    }

    return _batteryDischargeEnabled ? PL_UI_STATE_USE_SOLAR_AND_BATTERY : PL_UI_STATE_USE_SOLAR_ONLY;
}

int16_t PowerLimiterClass::calcHouseholdConsumption()
{
    auto const& config = Configuration.get();
    auto targetConsumption = config.PowerLimiter.TargetPowerConsumption;
    auto baseLoad = config.PowerLimiter.BaseLoadLimit;

    auto meterValid = PowerMeter.isDataValid();
    auto meterValue = PowerMeter.getPowerTotal();

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::calcHouseholdConsumption] target "
                "consumption: %d W, base load: %u W\r\n",
                targetConsumption, baseLoad);

        MessageOutput.printf("[DPL::calcHouseholdConsumption] power meter "
                "value: %.1f W, power meter valid: %s\r\n",
                meterValue, (meterValid?"yes":"no"));
    }

    if (!meterValid) { return baseLoad; }

    auto consumption = static_cast<int16_t>(meterValue + (meterValue > 0 ? 0.5 : -0.5));

    for (auto const& upInv : _inverters) {
        if (!upInv->isBehindPowerMeter()) { continue; }

        // If the inverter is wired behind the power meter, i.e., if its
        // output is part of the power meter measurement, the produced
        // power of this inverter has to be taken into account.
        auto invOutput = upInv->getCurrentOutputAcWatts();
        consumption += invOutput;
        if (_verboseLogging) {
            MessageOutput.printf("[DPL::calcHouseholdConsumption] inverter %s is "
                    "behind power meter producing %u W\r\n", upInv->getSerialStr(), invOutput);
        }
    }

    return consumption - targetConsumption;
}

/**
 * assigns new limits to all inverters matching the filter. returns the total
 * amount of power these inverters are expected to produce after the new limits
 * were applied.
 */
uint16_t PowerLimiterClass::updateInverterLimits(uint16_t powerRequested,
        PowerLimiterClass::inverter_filter_t filter, std::string const& filterExpression)
{
    std::vector<PowerLimiterInverter*> matchingInverters;
    uint16_t producing = 0; // sum of AC power the matching inverters produce now

    for (auto& upInv : _inverters) {
        if (!filter(*upInv)) { continue; }

        if (!upInv->isReachable()) {
            if (_verboseLogging) {
                MessageOutput.printf("[DPL::updateInverterLimits] skipping %s "
                        "as it is not reachable\r\n", upInv->getSerialStr());
            }
            continue;
        }

        if (!upInv->isSendingCommandsEnabled()) {
            if (_verboseLogging) {
                MessageOutput.printf("[DPL::updateInverterLimits] skipping %s "
                        "as sending commands is disabled\r\n", upInv->getSerialStr());
            }
            continue;
        }

        producing += upInv->getCurrentOutputAcWatts();
        matchingInverters.push_back(upInv.get());
    }

    int32_t diff = powerRequested - producing;

    auto const& config = Configuration.get();
    uint16_t hysteresis = config.PowerLimiter.TargetPowerConsumptionHysteresis;

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::updateInverterLimits] requested: %d W, "
                "producing: %d W using %d %s, diff: %i W, hysteresis: %d W\r\n",
                powerRequested, producing, matchingInverters.size(),
                filterExpression.c_str(), diff, hysteresis);
    }

    if (matchingInverters.empty()) { return 0; }

    if (std::abs(diff) < static_cast<int32_t>(hysteresis)) { return producing; }

    uint16_t covered = 0;

    if (diff < 0) {
        uint16_t reduction = static_cast<uint16_t>(diff * -1);

        uint16_t totalMaxReduction = 0;
        for (auto const pInv : matchingInverters) {
            totalMaxReduction += pInv->getMaxReductionWatts(false/*no standby*/);
        }

        // test whether we need to put at least one of the inverters into
        // standby to achieve the requested reduction.
        bool allowStandby = (totalMaxReduction < reduction);

        std::sort(matchingInverters.begin(), matchingInverters.end(),
                [allowStandby](auto const a, auto const b) {
                    auto aReduction = a->getMaxReductionWatts(allowStandby);
                    auto bReduction = b->getMaxReductionWatts(allowStandby);
                    return aReduction > bReduction;
                });

        for (auto pInv : matchingInverters) {
            auto maxReduction = pInv->getMaxReductionWatts(allowStandby);
            if (reduction >= hysteresis && maxReduction >= hysteresis) {
                reduction -= pInv->applyReduction(reduction, allowStandby);
            }
            covered += pInv->getExpectedOutputAcWatts();
        }
    }
    else {
        uint16_t increase = static_cast<uint16_t>(diff);

        std::sort(matchingInverters.begin(), matchingInverters.end(),
                [](auto const a, auto const b) {
                    return a->getMaxIncreaseWatts() > b->getMaxIncreaseWatts();
                });

        for (auto pInv : matchingInverters) {
            auto maxIncrease = pInv->getMaxIncreaseWatts();
            if (increase >= hysteresis && maxIncrease >= hysteresis) {
                increase -= pInv->applyIncrease(increase);
            }
            covered += pInv->getExpectedOutputAcWatts();
        }
    }

    if (_verboseLogging) {
        MessageOutput.printf("[DPL::updateInverterLimits] will cover %d W using "
                "%s\r\n", covered, filterExpression.c_str());
        for (auto pInv : matchingInverters) { pInv->debug(); }
    }

    return covered;
}

uint16_t PowerLimiterClass::calcBatteryAllowance(uint16_t powerRequested)
{
    if (_verboseLogging) {
        MessageOutput.printf("[DPL::calcBatteryAllowance] power requested: %d W\r\n",
                powerRequested);
    }

    // We check if the PSU is on and disable battery-powered inverters in this
    // case. The PSU should reduce power or shut down first before the
    // battery-powered inverters kick in. The only case where this is not
    // desired is if the battery is over the Full Solar Passthrough Threshold.
    // In this case battery-powered inverters should produce power and the PSU
    // will shut down as a consequence.
    if (!isFullSolarPassthroughActive() && HuaweiCan.getAutoPowerStatus()) {
        if (_verboseLogging) {
            MessageOutput.println("[DPL::calcBatteryAllowance] disabled "
                    "by HuaweiCan auto power");
        }
        return 0;
    }

    auto oBatteryPowerDc = getBatteryDischargeLimit();
    if (!oBatteryPowerDc.has_value()) { return powerRequested; }

    auto batteryPowerAC = solarDcToInverterAc(*oBatteryPowerDc);
    auto solarPowerAC = solarDcToInverterAc(getSolarPassthroughPower());

    if (powerRequested > batteryPowerAC + solarPowerAC) {
        // respect battery-provided discharge power limit
        auto res = batteryPowerAC + solarPowerAC;

        if (_verboseLogging) {
            MessageOutput.printf("[DPL::calcBatteryAllowance] limited by "
                    "battery (%d W) and/or solar power (%d W): %d W\r\n",
                    batteryPowerAC, solarPowerAC, res);
        }

        return res;
    }

    return powerRequested;
}

bool PowerLimiterClass::updateInverters()
{
    bool busy = false;

    for (auto& upInv : _inverters) {
        if (upInv->update()) { busy = true; }
    }

    return busy;
}

uint16_t PowerLimiterClass::getSolarPassthroughPower()
{
    auto const& config = Configuration.get();

    if (!config.PowerLimiter.SolarPassThroughEnabled
            || isBelowStopThreshold()
            || !VictronMppt.isDataValid()) {
        return 0;
    }

    return VictronMppt.getPowerOutputWatts();
}

float PowerLimiterClass::getBatteryInvertersOutputAcWatts()
{
    float res = 0;

    for (auto const& upInv : _inverters) {
        if (upInv->isSolarPowered()) { continue; }
        // TODO(schlimmchen): we must use the DC power instead, as the battery
        // voltage drops proportional to the DC current draw, but the AC power
        // output does not correlate with the battery current or voltage.
        res += upInv->getCurrentOutputAcWatts();
    }

    return res;
}

std::optional<uint16_t> PowerLimiterClass::getBatteryDischargeLimit()
{
    if (!_batteryDischargeEnabled) { return 0; }

    auto currentLimit = Battery.getDischargeCurrentLimit();
    if (currentLimit == FLT_MAX) { return std::nullopt; }

    if (currentLimit <= 0) { currentLimit = -currentLimit; }

    // this uses inverter voltage since there is a voltage drop between
    // battery and inverter, so since we are regulating the inverter
    // power we should use its voltage.
    auto inverter = getInverterDcVoltage();
    if (inverter.first <= 0) {
        MessageOutput.println("[DPL::getBatteryDischargeLimit]: could not "
                "determine inverter voltage");
        return 0;
    }

    return inverter.first * currentLimit;
}

float PowerLimiterClass::getLoadCorrectedVoltage()
{
    auto const& config = Configuration.get();

    // TODO(schlimmchen): use the battery's data if available,
    // i.e., the current drawn from the battery as reported by the battery.
    float acPower = getBatteryInvertersOutputAcWatts();
    float dcVoltage = getBatteryVoltage();

    if (dcVoltage <= 0.0) {
        return 0.0;
    }

    return dcVoltage + (acPower * config.PowerLimiter.VoltageLoadCorrectionFactor);
}

bool PowerLimiterClass::testThreshold(float socThreshold, float voltThreshold,
        std::function<bool(float, float)> compare)
{
    auto const& config = Configuration.get();

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
    auto const& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStartThreshold,
            config.PowerLimiter.VoltageStartThreshold,
            [](float a, float b) -> bool { return a >= b; }
    );
}

bool PowerLimiterClass::isStopThresholdReached()
{
    auto const& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStopThreshold,
            config.PowerLimiter.VoltageStopThreshold,
            [](float a, float b) -> bool { return a <= b; }
    );
}

bool PowerLimiterClass::isBelowStopThreshold()
{
    auto const& config = Configuration.get();

    return testThreshold(
            config.PowerLimiter.BatterySocStopThreshold,
            config.PowerLimiter.VoltageStopThreshold,
            [](float a, float b) -> bool { return a < b; }
    );
}

/// @brief calculate next inverter restart in millis
void PowerLimiterClass::calcNextInverterRestart()
{
    auto const& config = Configuration.get();

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

bool PowerLimiterClass::isFullSolarPassthroughActive()
{
    auto const& config = Configuration.get();

    // solar passthrough only applies to setups with battery-powered inverters
    if (!usesBatteryPoweredInverter()) { return false; }

    // We only do full solar PT if general solar PT is enabled
    if(!config.PowerLimiter.SolarPassThroughEnabled) { return false; }

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

bool PowerLimiterClass::usesBatteryPoweredInverter()
{
    for (auto const& upInv : _inverters) {
        if (!upInv->isSolarPowered()) { return true; }
    }

    return false;
}

bool PowerLimiterClass::isGovernedInverterProducing()
{
    for (auto const& upInv : _inverters) {
        if (upInv->isProducing()) { return true; }
    }
    return false;
}

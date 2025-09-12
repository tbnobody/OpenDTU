// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include <battery/Controller.h>
#include <gridcharger/huawei/Controller.h>
#include <gridcharger/huawei/MCP2515.h>
#include <gridcharger/huawei/TWAI.h>
#include <powermeter/Controller.h>
#include "PowerLimiter.h"
#include "Configuration.h"
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "Controller";

#include <functional>
#include <algorithm>

GridChargers::Huawei::Controller HuaweiCan;

namespace GridChargers::Huawei {

// Wait time/current before shuting down the PSU / charger
// This is set to allow the fan to run for some time
#define HUAWEI_AUTO_MODE_SHUTDOWN_DELAY 60000
#define HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT 0.75

void Controller::init(Scheduler& scheduler)
{
    DTU_LOGI("Initialize Huawei AC charger interface...");

    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    updateSettings();
}

void Controller::enableOutput()
{
    if (_oOutputEnabled.value_or(false)) { return; }

    _setProduction(true);
    _oOutputEnabled = true;

    if (_huaweiPower <= GPIO_NUM_NC) { return; }
    digitalWrite(_huaweiPower, 0);
}

void Controller::disableOutput()
{
    if (!_oOutputEnabled.value_or(true)) { return; }

    _setProduction(false);
    _oOutputEnabled = false;

    if (_huaweiPower <= GPIO_NUM_NC) { return; }
    digitalWrite(_huaweiPower, 1);
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _upHardwareInterface.reset(nullptr);

    auto const& config = Configuration.get();

    if (!config.GridCharger.Enabled) { return; }

    switch (config.GridCharger.Can.HardwareInterface) {
        case GridChargerHardwareInterface::MCP2515:
            _upHardwareInterface = std::make_unique<MCP2515>();
            break;
        case GridChargerHardwareInterface::TWAI:
            _upHardwareInterface = std::make_unique<TWAI>();
            break;
        default:
            DTU_LOGE("Unknown hardware interface setting %d", config.GridCharger.Can.HardwareInterface);
            return;
            break;
    }

    if (!_upHardwareInterface->init()) {
        DTU_LOGE("Error initializing hardware interface");
        _upHardwareInterface.reset(nullptr);
        return;
    };

    auto const& pin = PinMapping.get();
    if (pin.huawei_power > GPIO_NUM_NC) {
        _huaweiPower = pin.huawei_power;
        pinMode(_huaweiPower, OUTPUT);
        disableOutput();
    }

    _mode = HUAWEI_MODE_AUTO_EXT;
    if (config.GridCharger.AutoPowerEnabled) {
        _mode = HUAWEI_MODE_AUTO_INT;
    }

    DTU_LOGI("Hardware Interface initialized successfully");
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }

    auto const& config = Configuration.get();

    auto upNewData = _upHardwareInterface->getCurrentData();
    if (upNewData) {
        _dataPoints.updateFrom(*upNewData);
    }

    auto oOutputCurrent = _dataPoints.get<DataPointLabel::OutputCurrent>();
    auto oOutputVoltage = _dataPoints.get<DataPointLabel::OutputVoltage>();
    auto oOutputPower = _dataPoints.get<DataPointLabel::OutputPower>();
    auto oEfficiency = _dataPoints.get<DataPointLabel::Efficiency>();
    auto efficiency = oEfficiency ? (*oEfficiency > 50 ? *oEfficiency / 100 : 1.0) : 1.0;

    // Internal PSU power pin (slot detect) control
    if (oOutputCurrent && *oOutputCurrent > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT) {
        _outputCurrentOnSinceMillis = millis();
    }

    if (_outputCurrentOnSinceMillis + HUAWEI_AUTO_MODE_SHUTDOWN_DELAY < millis() &&
            (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
        disableOutput();
    }

    using Setting = HardwareInterface::Setting;

    if (_mode == HUAWEI_MODE_AUTO_INT || _batteryEmergencyCharging) {

        // Set voltage limit in periodic intervals if we're in auto mode or if emergency battery charge is requested.
        if ( _nextAutoModePeriodicIntMillis < millis()) {
            DTU_LOGI("Periodically setting voltage limit: %f", config.GridCharger.AutoPowerVoltageLimit);
            _setParameter(config.GridCharger.AutoPowerVoltageLimit, Setting::OnlineVoltage);
            _nextAutoModePeriodicIntMillis = millis() + 60000;
        }
    }

    // ***********************
    // Emergency charge
    // ***********************
    auto stats = Battery.getStats();
    if (!_batteryEmergencyCharging && config.GridCharger.EmergencyChargeEnabled && stats->getImmediateChargingRequest()) {
        if (!oOutputVoltage) {
            // TODO(schlimmchen): if this situation actually occurs, this message
            // will be printed with high frequency for a prolonged time. how can
            // we deal with that?
            DTU_LOGW("Cannot perform emergency charging with unknown PSU output voltage value");
            return;
        }

        _batteryEmergencyCharging = true;

        // Set output current
        float outputCurrent = config.GridCharger.AutoPowerUpperPowerLimit / *oOutputVoltage;
        DTU_LOGI("Emergency Charge Output current %.02f", outputCurrent);
        _setParameter(outputCurrent, Setting::OnlineCurrent);
        return;
    }

    if (_batteryEmergencyCharging && !stats->getImmediateChargingRequest()) {
        // Battery request has changed. Set current to 0, wait for PSU to respond and then clear state
        // TODO(schlimmchen): this is repeated very often for up to (polling interval) seconds. maybe
        // trigger sending request for data immediately? otherwise implement a backoff instead.
        _setParameter(0, Setting::OnlineCurrent);
        if (oOutputCurrent && *oOutputCurrent < 1) {
            _batteryEmergencyCharging = false;
        }
        return;
    }

    // ***********************
    // Automatic power control
    // ***********************
    if (_mode == HUAWEI_MODE_AUTO_INT ) {
        // Check if we should run automatic power calculation at all.
        // We may have set a value recently and still wait for output stabilization
        if (_autoModeBlockedTillMillis > millis()) {
            return;
        }

        if (!oOutputVoltage || !oOutputPower || !oOutputCurrent) {
            DTU_LOGW("Cannot perform auto power control while critical PSU values are still unknown");
            _autoModeBlockedTillMillis = millis() + 1000;
            return;
        }

        // Re-enable automatic power control if the output voltage has dropped below threshold
        if (oOutputVoltage && *oOutputVoltage < config.GridCharger.AutoPowerEnableVoltageLimit ) {
            _autoPowerEnabledCounter = 10;
        }

        if (PowerLimiter.isGovernedBatteryPoweredInverterProducing()) {
            _setParameter(0.0, Setting::OnlineCurrent);
            // Don't run auto mode for a second now. Otherwise we may send too much over the CAN bus
            _autoModeBlockedTillMillis = millis() + 1000;
            DTU_LOGI("Inverter is active, disable PSU");
            return;
        }

        if (PowerMeter.getLastUpdate() > _lastPowerMeterUpdateReceivedMillis &&
                _autoPowerEnabledCounter > 0) {
            // We have received a new PowerMeter value. Also we're _autoPowerEnabled
            // So we're good to calculate a new limit

            _lastPowerMeterUpdateReceivedMillis = PowerMeter.getLastUpdate();

            // input power diff will be (close to) zero if the power meter value
            // reads what the user specified as the target power consumption.
            float inputPowerDiff = -1 * round(PowerMeter.getPowerTotal());
            inputPowerDiff += config.GridCharger.AutoPowerTargetPowerConsumption;

            // target output power is current output adjusted for difference to desired input power
            float newOutputPowerTarget = *oOutputPower + (inputPowerDiff * efficiency);

            DTU_LOGD("input diff: %.0f, new output target: %.0f, current output: %.01f",
                inputPowerDiff, newOutputPowerTarget, *oOutputPower);

            // Check whether the battery SoC limit setting is enabled
            if (config.Battery.Enabled && config.GridCharger.AutoPowerBatterySoCLimitsEnabled) {
                uint8_t _batterySoC = Battery.getStats()->getSoC();
                // Sets power limit to 0 if the BMS reported SoC reaches or exceeds the user configured value
                if (_batterySoC >= config.GridCharger.AutoPowerStopBatterySoCThreshold) {
                    newOutputPowerTarget = 0;
                    DTU_LOGD("Current battery SoC %i reached stop threshold %i, so new output target is %f",
                            _batterySoC, config.GridCharger.AutoPowerStopBatterySoCThreshold, newOutputPowerTarget);
                }
            }

            if (newOutputPowerTarget > config.GridCharger.AutoPowerLowerPowerLimit) {
                // Check if the output power has dropped below the lower limit (i.e. the battery is full)
                // and if the PSU should be turned off. Also we use a simple counter mechanism here to be able
                // to ramp up from zero output power when starting up
                if (*oOutputPower < config.GridCharger.AutoPowerLowerPowerLimit) {
                    DTU_LOGI("Power and voltage limit reached. Disabling automatic power control.");
                    _autoPowerEnabledCounter--;
                    if (_autoPowerEnabledCounter == 0) {
                        _autoPowerEnabled = false;
                        _setParameter(0.0, Setting::OnlineCurrent);
                        return;
                    }
                } else {
                    _autoPowerEnabledCounter = 10;
                }

                newOutputPowerTarget = std::min(newOutputPowerTarget, config.GridCharger.AutoPowerUpperPowerLimit);

                float calculatedCurrent = newOutputPowerTarget / *oOutputVoltage;

                // Limit output current to value requested by BMS
                float permissibleCurrent = stats->getChargeCurrentLimit() - (stats->getChargeCurrent() - *oOutputCurrent); // BMS current limit - current from other sources, e.g. Victron MPPT charger
                float outputCurrent = std::min(calculatedCurrent, permissibleCurrent);
                outputCurrent = outputCurrent > 0 ? outputCurrent : 0;

                DTU_LOGD("Setting output current to %.2fA. This is the lower value of "
                        "calculated %.2fA and BMS permissable %.2fA currents",
                        outputCurrent, calculatedCurrent, permissibleCurrent);

                _autoPowerEnabled = true;
                _setParameter(outputCurrent, Setting::OnlineCurrent);

                // Don't run auto mode some time to allow for output stabilization after issuing a new value
                _autoModeBlockedTillMillis = millis() + 2 * HardwareInterface::DataRequestIntervalMillis;
            } else {
                // requested PL is below minium. Set current to 0
                _autoPowerEnabled = false;
                _setParameter(0.0, Setting::OnlineCurrent);
            }
        }
    }
}

void Controller::setFan(bool online, bool fullSpeed)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }

    using Setting = HardwareInterface::Setting;
    auto setting = online ? Setting::FanOnlineFullSpeed : Setting::FanOfflineFullSpeed;
    _upHardwareInterface->setParameter(setting, fullSpeed ? 1 : 0);
}

void Controller::_setProduction(bool enable)
{
    auto setting = HardwareInterface::Setting::ProductionDisable;
    _upHardwareInterface->setParameter(setting, enable ? 0 : 1);
}

void Controller::setProduction(bool enable)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }
    _setProduction(enable);
}

void Controller::setParameter(float val, HardwareInterface::Setting setting)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_mode == HUAWEI_MODE_AUTO_INT &&
        setting != HardwareInterface::Setting::OfflineVoltage &&
        setting != HardwareInterface::Setting::OfflineCurrent) { return; }

    // set pollFeedback to true only if we're setting a value because of a
    // request from the UI or MQTT. otherwise, we'll wait for the next
    // interval to get the new value.
    _setParameter(val, setting, true/*pollFeedback*/);
}

void Controller::_setParameter(float val, HardwareInterface::Setting setting, bool pollFeedback)
{
    // NOTE: the mutex is locked by any method calling this private method

    if (!_upHardwareInterface) { return; }

    if (val < 0) {
        DTU_LOGE("Tried to set voltage/current to negative value %.2f", val);
        return;
    }

    using Setting = HardwareInterface::Setting;

    // Start PSU if needed
    if (val > HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT &&
            setting == Setting::OnlineCurrent &&
            (_mode == HUAWEI_MODE_AUTO_EXT || _mode == HUAWEI_MODE_AUTO_INT)) {
        enableOutput();
        _outputCurrentOnSinceMillis = millis();
    }

    _upHardwareInterface->setParameter(setting, val, pollFeedback);
}

void Controller::setMode(uint8_t mode) {
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }

    if (mode == HUAWEI_MODE_OFF) {
        disableOutput();
        _mode = HUAWEI_MODE_OFF;
    }
    if (mode == HUAWEI_MODE_ON) {
        enableOutput();
        _mode = HUAWEI_MODE_ON;
    }

    auto const& config = Configuration.get();

    if (mode == HUAWEI_MODE_AUTO_INT && !config.GridCharger.AutoPowerEnabled ) {
        DTU_LOGW("Trying to set mode to internal automatic power control "
                "without being enabled in the UI. Ignoring command.");
        return;
    }

    if (_mode == HUAWEI_MODE_AUTO_INT && mode != HUAWEI_MODE_AUTO_INT) {
        _autoPowerEnabled = false;
        _setParameter(0, HardwareInterface::Setting::OnlineCurrent);
    }

    if (mode == HUAWEI_MODE_AUTO_EXT || mode == HUAWEI_MODE_AUTO_INT) {
        _mode = mode;
    }
}

void Controller::getJsonData(JsonVariant& root) const
{
    root["dataAge"] = millis() - _dataPoints.getLastUpdate();

    using Label = GridChargers::Huawei::DataPointLabel;

    auto oReachable = _dataPoints.get<Label::Reachable>();
    root["reachable"] = oReachable.value_or(false);

    auto oOutputPower = _dataPoints.get<Label::OutputPower>();
    auto oOutputCurrent = _dataPoints.get<Label::OutputCurrent>();
    root["producing"] = oOutputPower.value_or(0) > 10 && oOutputCurrent.value_or(0) > 0.1;

#define VAL(l, n) \
    { \
        auto oVal = _dataPoints.get<Label::l>(); \
        if (oVal) { root[n] = *oVal; } \
    }

    VAL(Serial,              "serial");
    VAL(VendorName,          "vendorName");
    VAL(ProductName,         "productName");
#undef VAL

    addStringInSection<Label::BoardType>(root, "device", "boardType");
    addStringInSection<Label::Manufactured>(root, "device", "manufactured");
    addStringInSection<Label::ProductDescription>(root, "device", "productDescription");
    addStringInSection<Label::Row>(root, "device", "row");
    addStringInSection<Label::Slot>(root, "device", "slot");

    addValueInSection<Label::InputVoltage>(root, "input", "voltage");
    addValueInSection<Label::InputCurrent>(root, "input", "current");
    addValueInSection<Label::InputPower>(root, "input", "power");
    addValueInSection<Label::InputTemperature>(root, "input", "temp");
    addValueInSection<Label::InputFrequency>(root, "input", "frequency");
    addValueInSection<Label::Efficiency>(root, "input", "efficiency");

    addValueInSection<Label::OutputVoltage>(root, "output", "voltage");
    addValueInSection<Label::OutputCurrent>(root, "output", "current");
    addValueInSection<Label::OutputPower>(root, "output", "power");
    addValueInSection<Label::OutputTemperature>(root, "output", "temp");
    addValueInSection<Label::OutputCurrentMax>(root, "output", "maxCurrent");

    addValueInSection<Label::OnlineVoltage>(root, "acknowledgements", "onlineVoltage");
    addValueInSection<Label::OfflineVoltage>(root, "acknowledgements", "offlineVoltage");
    addValueInSection<Label::OnlineCurrent>(root, "acknowledgements", "onlineCurrent");
    addValueInSection<Label::OfflineCurrent>(root, "acknowledgements", "offlineCurrent");
    addValueInSection<Label::InputCurrentLimit>(root, "acknowledgements", "inputCurrentLimit");

    auto oProductionEnabled = _dataPoints.get<Label::ProductionEnabled>();
    if (oProductionEnabled) {
        addStringInSection(root, "acknowledgements", "productionEnabled", *oProductionEnabled?"yes":"no");
    }

    auto oFanOnlineFullSpeed = _dataPoints.get<Label::FanOnlineFullSpeed>();
    if (oFanOnlineFullSpeed) {
        addStringInSection(root, "acknowledgements", "fanOnlineFullSpeed", *oFanOnlineFullSpeed?"FanFullSpeed":"FanAuto");
    }

    auto oFanOfflineFullSpeed = _dataPoints.get<Label::FanOfflineFullSpeed>();
    if (oFanOfflineFullSpeed) {
        addStringInSection(root, "acknowledgements", "fanOfflineFullSpeed", *oFanOfflineFullSpeed?"FanFullSpeed":"FanAuto");
    }
}

} // namespace GridChargers::Huawei

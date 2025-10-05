// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include <battery/Controller.h>
#include <gridcharger/huawei/Provider.h>
#include <gridcharger/huawei/MCP2515.h>
#include <gridcharger/huawei/TWAI.h>
#include <powermeter/Controller.h>
#include <PowerLimiter.h>
#include <Configuration.h>
#include <LogHelper.h>
#include <MqttSettings.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "Huawei";

#include <functional>
#include <algorithm>

namespace GridChargers::Huawei {

// Wait time/current before shuting down the PSU / charger
// This is set to allow the fan to run for some time
#define HUAWEI_AUTO_MODE_SHUTDOWN_DELAY 60000
#define HUAWEI_AUTO_MODE_SHUTDOWN_CURRENT 0.75

bool Provider::init()
{
    DTU_LOGI("Initialize Huawei AC charger interface...");

    _upHardwareInterface.reset(nullptr);

    auto const& config = Configuration.get();

    switch (config.GridCharger.Can.HardwareInterface) {
        case GridChargerHardwareInterface::MCP2515:
            _upHardwareInterface = std::make_unique<MCP2515>();
            break;
        case GridChargerHardwareInterface::TWAI:
            _upHardwareInterface = std::make_unique<TWAI>();
            break;
        default:
            DTU_LOGE("Unknown hardware interface setting %d", config.GridCharger.Can.HardwareInterface);
            return false;
            break;
    }

    if (!_upHardwareInterface->init()) {
        DTU_LOGE("Error initializing hardware interface");
        _upHardwareInterface.reset(nullptr);
        return false;
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

    // Set initial mode in datapoints
    _dataPoints.add<DataPointLabel::Mode>(_mode, true);

    subscribeTopics();

    DTU_LOGI("Hardware Interface initialized successfully");
    return true;
}

void Provider::deinit()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _upHardwareInterface.reset(nullptr);
    unsubscribeTopics();
}

void Provider::enableOutput()
{
    if (_oOutputEnabled.value_or(false)) { return; }

    _setProduction(true);
    _oOutputEnabled = true;

    if (_huaweiPower <= GPIO_NUM_NC) { return; }
    digitalWrite(_huaweiPower, 0);
}

void Provider::disableOutput()
{
    if (!_oOutputEnabled.value_or(true)) { return; }

    _setProduction(false);
    _oOutputEnabled = false;

    if (_huaweiPower <= GPIO_NUM_NC) { return; }
    digitalWrite(_huaweiPower, 1);
}

void Provider::subscribeTopics()
{
    String const& prefix = MqttSettings.getPrefix();

    auto subscribe = [&prefix, this](char const* subTopic, Topic t) {
        String fullTopic(prefix + _cmdtopic.data() + subTopic);
        MqttSettings.subscribe(fullTopic.c_str(), 0,
                std::bind(&Provider::onMqttMessage, this, t,
                    std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4));
    };

    for (auto const& s : _subscriptions) {
        subscribe(s.first.data(), s.second);
    }
}

void Provider::unsubscribeTopics()
{
    String const prefix = MqttSettings.getPrefix() + _cmdtopic.data();
    for (auto const& s : _subscriptions) {
        MqttSettings.unsubscribe(prefix + s.first.data());
    }
}

void Provider::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }

    auto const& config = Configuration.get();

    if (!config.GridCharger.Enabled) {
        return;
    }

    auto upNewData = _upHardwareInterface->getCurrentData();
    if (upNewData) {
        _dataPoints.updateFrom(*upNewData);
        _stats->updateFrom(*upNewData);
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
                        "calculated %.2fA and BMS permissible %.2fA currents",
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

void Provider::setFan(bool online, bool fullSpeed)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }

    using Setting = HardwareInterface::Setting;
    auto setting = online ? Setting::FanOnlineFullSpeed : Setting::FanOfflineFullSpeed;
    _upHardwareInterface->setParameter(setting, fullSpeed ? 1 : 0);
}

void Provider::_setProduction(bool enable) const
{
    auto setting = HardwareInterface::Setting::ProductionDisable;
    _upHardwareInterface->setParameter(setting, enable ? 0 : 1);
}

void Provider::setProduction(bool enable)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upHardwareInterface) { return; }
    _setProduction(enable);
}

void Provider::setParameter(float val, HardwareInterface::Setting setting)
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

void Provider::_setParameter(float val, HardwareInterface::Setting setting, bool pollFeedback)
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

void Provider::setMode(uint8_t mode) {
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

    // Update mode in datapoints
    _dataPoints.add<DataPointLabel::Mode>(_mode, true);

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
        // Update mode in datapoints for AUTO modes too
        _dataPoints.add<DataPointLabel::Mode>(_mode, true);
    }
}

void Provider::onMqttMessage(Topic enumTopic,
        const espMqttClientTypes::MessageProperties& properties,
        const char* topic, const uint8_t* payload, size_t len)
{
    std::string strValue(reinterpret_cast<const char*>(payload), len);
    float payload_val = -1;
    try {
        payload_val = std::stof(strValue);
    }
    catch (std::invalid_argument const& e) {
        DTU_LOGE("Huawei MQTT handler: cannot parse payload of topic '%s' as float: %s",
                topic, strValue.c_str());
        return;
    }

    using Setting = HardwareInterface::Setting;

    auto validateAndSetParameter = [this, payload_val](float min, float max,
            Setting setting, const char* paramName, const char* unit) -> bool {
        if (payload_val < min || payload_val > max) {
            DTU_LOGE("Invalid %s %.2f %s (valid range: %.2f-%.2f %s)",
                paramName, payload_val, unit, min, max, unit);
            return false;
        }
        DTU_LOGI("Limit %s: %.2f %s", paramName, payload_val, unit);
        setParameter(payload_val, setting);
        return true;
    };

    switch (enumTopic) {
        case Topic::LimitOnlineVoltage:
            validateAndSetParameter(MIN_ONLINE_VOLTAGE, MAX_ONLINE_VOLTAGE,
                Setting::OnlineVoltage, "online voltage", "V");
            break;

        case Topic::LimitOfflineVoltage:
            validateAndSetParameter(MIN_OFFLINE_VOLTAGE, MAX_OFFLINE_VOLTAGE,
                Setting::OfflineVoltage, "offline voltage", "V");
            break;

        case Topic::LimitOnlineCurrent:
            validateAndSetParameter(MIN_ONLINE_CURRENT, MAX_ONLINE_CURRENT,
                Setting::OnlineCurrent, "online current", "A");
            break;

        case Topic::LimitOfflineCurrent:
            validateAndSetParameter(MIN_OFFLINE_CURRENT, MAX_OFFLINE_CURRENT,
                Setting::OfflineCurrent, "offline current", "A");
            break;

        case Topic::Mode:
            switch (static_cast<int>(payload_val)) {
                case 3:
                    DTU_LOGI("Received MQTT msg. New mode: Full internal control");
                    setMode(HUAWEI_MODE_AUTO_INT);
                    break;

                case 2:
                    DTU_LOGI("Received MQTT msg. New mode: Internal on/off control, external power limit");
                    setMode(HUAWEI_MODE_AUTO_EXT);
                    break;

                case 1:
                    DTU_LOGI("Received MQTT msg. New mode: Turned ON");
                    setMode(HUAWEI_MODE_ON);
                    break;

                case 0:
                    DTU_LOGI("Received MQTT msg. New mode: Turned OFF");
                    setMode(HUAWEI_MODE_OFF);
                    break;

                default:
                    DTU_LOGE("Invalid mode %.0f", payload_val);
                    break;
            }
            break;

        case Topic::Production:
        {
            bool enable = payload_val > 0;
            DTU_LOGI("Production to be %sabled", (enable?"en":"dis"));
            setProduction(enable);
            break;
        }

        case Topic::LimitInputCurrent:
            validateAndSetParameter(MIN_INPUT_CURRENT_LIMIT, MAX_INPUT_CURRENT_LIMIT,
                Setting::InputCurrentLimit, "input current", "A");
            break;

        case Topic::FanOnlineFullSpeed:
        case Topic::FanOfflineFullSpeed:
        {
            bool online = (Topic::FanOnlineFullSpeed == enumTopic);
            bool fullSpeed = payload_val > 0;
            setFan(online, fullSpeed);
            break;
        }
    }
}

} // namespace GridChargers::Huawei

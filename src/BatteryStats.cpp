// SPDX-License-Identifier: GPL-2.0-or-later
#include <vector>
#include <algorithm>
#include "BatteryStats.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "JkBmsDataPoints.h"
#include "MqttSettings.h"

template<typename T>
static void addLiveViewInSection(JsonVariant& root,
    std::string const& section, std::string const& name,
    T&& value, std::string const& unit, uint8_t precision)
{
    auto jsonValue = root["values"][section][name];
    jsonValue["v"] = value;
    jsonValue["u"] = unit;
    jsonValue["d"] = precision;
}

template<typename T>
static void addLiveViewValue(JsonVariant& root, std::string const& name,
    T&& value, std::string const& unit, uint8_t precision)
{
    addLiveViewInSection(root, "status", name, value, unit, precision);
}

static void addLiveViewTextInSection(JsonVariant& root,
    std::string const& section, std::string const& name,
    std::string const& text, bool translate = true)
{
    auto jsonValue = root["values"][section][name];
    jsonValue["value"] = text;
    jsonValue["translate"] = translate;
}

static void addLiveViewTextValue(JsonVariant& root, std::string const& name,
    std::string const& text)
{
    addLiveViewTextInSection(root, "status", name, text);
}

static void addLiveViewWarning(JsonVariant& root, std::string const& name,
    bool warning)
{
    if (!warning) { return; }
    root["issues"][name] = 1;
}

static void addLiveViewAlarm(JsonVariant& root, std::string const& name,
    bool alarm)
{
    if (!alarm) { return; }
    root["issues"][name] = 2;
}

void BatteryStats::setManufacturer(const String& m)
{
    String sanitized(m);
    for (int i = 0; i < sanitized.length(); i++) {
        char c = sanitized[i];
        if (c < 0x20 || c >= 0x80) {
            sanitized.remove(i); // Truncate string
            break;
        }
    }
    _manufacturer = std::move(sanitized);
}

bool BatteryStats::updateAvailable(uint32_t since) const
{
    if (_lastUpdate == 0) { return false; } // no data at all processed yet

    auto constexpr halfOfAllMillis = std::numeric_limits<uint32_t>::max() / 2;
    return (_lastUpdate - since) < halfOfAllMillis;
}

void BatteryStats::getLiveViewData(JsonVariant& root) const
{
    root["manufacturer"] = _manufacturer;
    if (!_serial.isEmpty()) {
        root["serial"] = _serial;
    }
    if (!_fwversion.isEmpty()) {
        root["fwversion"] = _fwversion;
    }
    if (!_hwversion.isEmpty()) {
        root["hwversion"] = _hwversion;
    }
    root["data_age"] = getAgeSeconds();

    if (isSoCValid()) {
        addLiveViewValue(root, "SoC", _soc, "%", _socPrecision);
    }

    if (isVoltageValid()) {
        addLiveViewValue(root, "voltage", _voltage, "V", 2);
    }

    if (isCurrentValid()) {
        addLiveViewValue(root, "current", _current, "A", _currentPrecision);
    }

    if (isDischargeCurrentLimitValid()) {
        addLiveViewValue(root, "dischargeCurrentLimitation", _dischargeCurrentLimit, "A", 1);
    }

    root["showIssues"] = supportsAlarmsAndWarnings();
}

void MqttBatteryStats::getLiveViewData(JsonVariant& root) const
{
    // as we don't want to repeat the data that is already shown in the live data card
    // we only add the live view data here when the discharge current limit can be shown
    if (isDischargeCurrentLimitValid()) {
        BatteryStats::getLiveViewData(root);
    }
}

void PylontechBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltage, "V", 1);
    addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "dischargeVoltageLimitation", _dischargeVoltageLimitation, "V", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
    addLiveViewValue(root, "temperature", _temperature, "°C", 1);
    addLiveViewValue(root, "modules", _moduleCount, "", 0);

    addLiveViewTextValue(root, "chargeEnabled", (_chargeEnabled?"yes":"no"));
    addLiveViewTextValue(root, "dischargeEnabled", (_dischargeEnabled?"yes":"no"));
    addLiveViewTextValue(root, "chargeImmediately", (_chargeImmediately?"yes":"no"));

    // alarms and warnings go into the "Issues" card of the web application
    addLiveViewWarning(root, "highCurrentDischarge", _warningHighCurrentDischarge);
    addLiveViewAlarm(root, "overCurrentDischarge", _alarmOverCurrentDischarge);

    addLiveViewWarning(root, "highCurrentCharge", _warningHighCurrentCharge);
    addLiveViewAlarm(root, "overCurrentCharge", _alarmOverCurrentCharge);

    addLiveViewWarning(root, "lowTemperature", _warningLowTemperature);
    addLiveViewAlarm(root, "underTemperature", _alarmUnderTemperature);

    addLiveViewWarning(root, "highTemperature", _warningHighTemperature);
    addLiveViewAlarm(root, "overTemperature", _alarmOverTemperature);

    addLiveViewWarning(root, "lowVoltage", _warningLowVoltage);
    addLiveViewAlarm(root, "underVoltage", _alarmUnderVoltage);

    addLiveViewWarning(root, "highVoltage", _warningHighVoltage);
    addLiveViewAlarm(root, "overVoltage", _alarmOverVoltage);

    addLiveViewWarning(root, "bmsInternal", _warningBmsInternal);
    addLiveViewAlarm(root, "bmsInternal", _alarmBmsInternal);
}

void SBSBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltage, "V", 1);
    addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
    addLiveViewValue(root, "current", _current, "A", 1);
    addLiveViewValue(root, "temperature", _temperature, "°C", 1);
    addLiveViewTextValue(root, "chargeEnabled", (_chargeEnabled?"yes":"no"));
    addLiveViewTextValue(root, "dischargeEnabled", (_dischargeEnabled?"yes":"no"));

    // alarms and warnings go into the "Issues" card of the web application
    addLiveViewWarning(root, "highCurrentDischarge", _warningHighCurrentDischarge);
    addLiveViewWarning(root, "highCurrentCharge", _warningHighCurrentCharge);
    addLiveViewAlarm(root, "underVoltage", _alarmUnderVoltage);
    addLiveViewAlarm(root, "overVoltage", _alarmOverVoltage);
    addLiveViewAlarm(root, "bmsInternal", _alarmBmsInternal);
    addLiveViewAlarm(root, "underTemperature", _alarmUnderTemperature);
    addLiveViewAlarm(root, "overTemperature", _alarmOverTemperature);
}

void PytesBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltageLimit, "V", 1);
    addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimit, "A", 1);
    addLiveViewValue(root, "dischargeVoltageLimitation", _dischargeVoltageLimit, "V", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
    if (_chargeCycles != -1) {
        addLiveViewValue(root, "chargeCycles", _chargeCycles, "", 0);
    }
    addLiveViewValue(root, "temperature", _temperature, "°C", 1);

    addLiveViewValue(root, "capacity", _totalCapacity, "Ah", _capacityPrecision);
    addLiveViewValue(root, "availableCapacity", _availableCapacity, "Ah", _capacityPrecision);

    if (_chargedEnergy != -1) {
        addLiveViewValue(root, "chargedEnergy", _chargedEnergy, "kWh", 1);
    }

    if (_dischargedEnergy != -1) {
        addLiveViewValue(root, "dischargedEnergy", _dischargedEnergy, "kWh", 1);
    }
    addLiveViewTextValue(root, "chargeImmediately", (_chargeImmediately?"yes":"no"));

    if (_balance != -1) {
        addLiveViewTextValue(root, "balancingActive", (_balance?"yes":"no"));
    }

    addLiveViewInSection(root, "cells", "cellMinVoltage", static_cast<float>(_cellMinMilliVolt)/1000, "V", 3);
    addLiveViewInSection(root, "cells", "cellMaxVoltage", static_cast<float>(_cellMaxMilliVolt)/1000, "V", 3);
    addLiveViewInSection(root, "cells", "cellDiffVoltage", (_cellMaxMilliVolt - _cellMinMilliVolt), "mV", 0);
    addLiveViewInSection(root, "cells", "cellMinTemperature", _cellMinTemperature, "°C", 0);
    addLiveViewInSection(root, "cells", "cellMaxTemperature", _cellMaxTemperature, "°C", 0);

    addLiveViewTextInSection(root, "cells", "cellMinVoltageName", _cellMinVoltageName.c_str(), false);
    addLiveViewTextInSection(root, "cells", "cellMaxVoltageName", _cellMaxVoltageName.c_str(), false);
    addLiveViewTextInSection(root, "cells", "cellMinTemperatureName", _cellMinTemperatureName.c_str(), false);
    addLiveViewTextInSection(root, "cells", "cellMaxTemperatureName", _cellMaxTemperatureName.c_str(), false);

    addLiveViewInSection(root, "modules", "online", _moduleCountOnline, "", 0);
    addLiveViewInSection(root, "modules", "offline", _moduleCountOffline, "", 0);
    addLiveViewInSection(root, "modules", "blockingCharge", _moduleCountBlockingCharge, "", 0);
    addLiveViewInSection(root, "modules", "blockingDischarge", _moduleCountBlockingDischarge, "", 0);

    // alarms and warnings go into the "Issues" card of the web application
    addLiveViewWarning(root, "highCurrentDischarge", _warningHighDischargeCurrent);
    addLiveViewAlarm(root, "overCurrentDischarge", _alarmOverCurrentDischarge);

    addLiveViewWarning(root, "highCurrentCharge", _warningHighChargeCurrent);
    addLiveViewAlarm(root, "overCurrentCharge", _alarmOverCurrentCharge);

    addLiveViewWarning(root, "lowVoltage", _warningLowVoltage);
    addLiveViewAlarm(root, "underVoltage", _alarmUnderVoltage);

    addLiveViewWarning(root, "highVoltage", _warningHighVoltage);
    addLiveViewAlarm(root, "overVoltage", _alarmOverVoltage);

    addLiveViewWarning(root, "lowTemperature", _warningLowTemperature);
    addLiveViewAlarm(root, "underTemperature", _alarmUnderTemperature);

    addLiveViewWarning(root, "highTemperature", _warningHighTemperature);
    addLiveViewAlarm(root, "overTemperature", _alarmOverTemperature);

    addLiveViewWarning(root, "lowTemperatureCharge", _warningLowTemperatureCharge);
    addLiveViewAlarm(root, "underTemperatureCharge", _alarmUnderTemperatureCharge);

    addLiveViewWarning(root, "highTemperatureCharge", _warningHighTemperatureCharge);
    addLiveViewAlarm(root, "overTemperatureCharge", _alarmOverTemperatureCharge);

    addLiveViewWarning(root, "bmsInternal", _warningInternalFailure);
    addLiveViewAlarm(root, "bmsInternal", _alarmInternalFailure);

    addLiveViewWarning(root, "cellDiffVoltage", _warningCellImbalance);
    addLiveViewAlarm(root, "cellDiffVoltage", _alarmCellImbalance);
}

void JkBmsBatteryStats::getJsonData(JsonVariant& root, bool verbose) const
{
    BatteryStats::getLiveViewData(root);

    using Label = JkBms::DataPointLabel;

    auto oCurrent = _dataPoints.get<Label::BatteryCurrentMilliAmps>();
    auto oVoltage = _dataPoints.get<Label::BatteryVoltageMilliVolt>();
    if (oVoltage.has_value() && oCurrent.has_value()) {
        auto current = static_cast<float>(*oCurrent) / 1000;
        auto voltage = static_cast<float>(*oVoltage) / 1000;
        addLiveViewValue(root, "power", current * voltage , "W", 2);
    }

    auto oTemperatureBms = _dataPoints.get<Label::BmsTempCelsius>();
    if (oTemperatureBms.has_value()) {
        addLiveViewValue(root, "bmsTemp", *oTemperatureBms, "°C", 0);
    }

    // labels BatteryChargeEnabled, BatteryDischargeEnabled, and
    // BalancingEnabled refer to the user setting. we want to show the
    // actual MOSFETs' state which control whether charging and discharging
    // is possible and whether the BMS is currently balancing cells.
    auto oStatus = _dataPoints.get<Label::StatusBitmask>();
    if (oStatus.has_value()) {
        using Bits = JkBms::StatusBits;
        auto chargeEnabled = *oStatus & static_cast<uint16_t>(Bits::ChargingActive);
        addLiveViewTextValue(root, "chargeEnabled", (chargeEnabled?"yes":"no"));
        auto dischargeEnabled = *oStatus & static_cast<uint16_t>(Bits::DischargingActive);
        addLiveViewTextValue(root, "dischargeEnabled", (dischargeEnabled?"yes":"no"));
    }

    auto oTemperatureOne = _dataPoints.get<Label::BatteryTempOneCelsius>();
    if (oTemperatureOne.has_value()) {
        addLiveViewInSection(root, "cells", "batOneTemp", *oTemperatureOne, "°C", 0);
    }

    auto oTemperatureTwo = _dataPoints.get<Label::BatteryTempTwoCelsius>();
    if (oTemperatureTwo.has_value()) {
        addLiveViewInSection(root, "cells", "batTwoTemp", *oTemperatureTwo, "°C", 0);
    }

    if (_cellVoltageTimestamp > 0) {
        addLiveViewInSection(root, "cells", "cellMinVoltage", static_cast<float>(_cellMinMilliVolt)/1000, "V", 3);
        addLiveViewInSection(root, "cells", "cellAvgVoltage", static_cast<float>(_cellAvgMilliVolt)/1000, "V", 3);
        addLiveViewInSection(root, "cells", "cellMaxVoltage", static_cast<float>(_cellMaxMilliVolt)/1000, "V", 3);
        addLiveViewInSection(root, "cells", "cellDiffVoltage", (_cellMaxMilliVolt - _cellMinMilliVolt), "mV", 0);
    }

    if (oStatus.has_value()) {
        using Bits = JkBms::StatusBits;
        auto balancingActive = *oStatus & static_cast<uint16_t>(Bits::BalancingActive);
        addLiveViewTextInSection(root, "cells", "balancingActive", (balancingActive?"yes":"no"));
    }

    auto oAlarms = _dataPoints.get<Label::AlarmsBitmask>();
    if (oAlarms.has_value()) {
#define ISSUE(t, x) \
        auto x = *oAlarms & static_cast<uint16_t>(JkBms::AlarmBits::x); \
        addLiveView##t(root, "JkBmsIssue"#x, x > 0);

        ISSUE(Warning, LowCapacity);
        ISSUE(Alarm, BmsOvertemperature);
        ISSUE(Alarm, ChargingOvervoltage);
        ISSUE(Alarm, DischargeUndervoltage);
        ISSUE(Alarm, BatteryOvertemperature);
        ISSUE(Alarm, ChargingOvercurrent);
        ISSUE(Alarm, DischargeOvercurrent);
        ISSUE(Alarm, CellVoltageDifference);
        ISSUE(Alarm, BatteryBoxOvertemperature);
        ISSUE(Alarm, BatteryUndertemperature);
        ISSUE(Alarm, CellOvervoltage);
        ISSUE(Alarm, CellUndervoltage);
        ISSUE(Alarm, AProtect);
        ISSUE(Alarm, BProtect);
#undef ISSUE
    }
}

void BatteryStats::mqttLoop()
{
    auto& config = Configuration.get();

    if (!MqttSettings.getConnected()
            || (millis() - _lastMqttPublish) < (config.Mqtt.PublishInterval * 1000)) {
        return;
    }

    mqttPublish();

    _lastMqttPublish = millis();
}

uint32_t BatteryStats::getMqttFullPublishIntervalMs() const
{
    auto& config = Configuration.get();

    // this is the default interval, see mqttLoop(). mqttPublish()
    // implementations in derived classes may choose to publish some values
    // with a lower frequency and hence implement this method with a different
    // return value.
    return config.Mqtt.PublishInterval * 1000;
}

void BatteryStats::mqttPublish() const
{
    MqttSettings.publish("battery/manufacturer", _manufacturer);
    MqttSettings.publish("battery/dataAge", String(getAgeSeconds()));

    if (isSoCValid()) {
        MqttSettings.publish("battery/stateOfCharge", String(_soc));
    }

    if (isVoltageValid()) {
        MqttSettings.publish("battery/voltage", String(_voltage));
    }

    if (isCurrentValid()) {
        MqttSettings.publish("battery/current", String(_current));
    }

    if (isDischargeCurrentLimitValid()) {
        MqttSettings.publish("battery/settings/dischargeCurrentLimitation", String(_dischargeCurrentLimit));
    }
}

void PylontechBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    MqttSettings.publish("battery/settings/chargeVoltage", String(_chargeVoltage));
    MqttSettings.publish("battery/settings/chargeCurrentLimitation", String(_chargeCurrentLimitation));
    MqttSettings.publish("battery/settings/dischargeVoltageLimitation", String(_dischargeVoltageLimitation));
    MqttSettings.publish("battery/stateOfHealth", String(_stateOfHealth));
    MqttSettings.publish("battery/temperature", String(_temperature));
    MqttSettings.publish("battery/alarm/overCurrentDischarge", String(_alarmOverCurrentDischarge));
    MqttSettings.publish("battery/alarm/overCurrentCharge", String(_alarmOverCurrentCharge));
    MqttSettings.publish("battery/alarm/underTemperature", String(_alarmUnderTemperature));
    MqttSettings.publish("battery/alarm/overTemperature", String(_alarmOverTemperature));
    MqttSettings.publish("battery/alarm/underVoltage", String(_alarmUnderVoltage));
    MqttSettings.publish("battery/alarm/overVoltage", String(_alarmOverVoltage));
    MqttSettings.publish("battery/alarm/bmsInternal", String(_alarmBmsInternal));
    MqttSettings.publish("battery/warning/highCurrentDischarge", String(_warningHighCurrentDischarge));
    MqttSettings.publish("battery/warning/highCurrentCharge", String(_warningHighCurrentCharge));
    MqttSettings.publish("battery/warning/lowTemperature", String(_warningLowTemperature));
    MqttSettings.publish("battery/warning/highTemperature", String(_warningHighTemperature));
    MqttSettings.publish("battery/warning/lowVoltage", String(_warningLowVoltage));
    MqttSettings.publish("battery/warning/highVoltage", String(_warningHighVoltage));
    MqttSettings.publish("battery/warning/bmsInternal", String(_warningBmsInternal));
    MqttSettings.publish("battery/charging/chargeEnabled", String(_chargeEnabled));
    MqttSettings.publish("battery/charging/dischargeEnabled", String(_dischargeEnabled));
    MqttSettings.publish("battery/charging/chargeImmediately", String(_chargeImmediately));
    MqttSettings.publish("battery/modulesTotal", String(_moduleCount));
}

void SBSBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    MqttSettings.publish("battery/settings/chargeVoltage", String(_chargeVoltage));
    MqttSettings.publish("battery/settings/chargeCurrentLimitation", String(_chargeCurrentLimitation));
    MqttSettings.publish("battery/stateOfHealth", String(_stateOfHealth));
    MqttSettings.publish("battery/current", String(_current));
    MqttSettings.publish("battery/temperature", String(_temperature));
    MqttSettings.publish("battery/alarm/underVoltage", String(_alarmUnderVoltage));
    MqttSettings.publish("battery/alarm/overVoltage", String(_alarmOverVoltage));
    MqttSettings.publish("battery/alarm/bmsInternal", String(_alarmBmsInternal));
    MqttSettings.publish("battery/warning/highCurrentDischarge", String(_warningHighCurrentDischarge));
    MqttSettings.publish("battery/warning/highCurrentCharge", String(_warningHighCurrentCharge));
    MqttSettings.publish("battery/charging/chargeEnabled", String(_chargeEnabled));
    MqttSettings.publish("battery/charging/dischargeEnabled", String(_dischargeEnabled));
}

void PytesBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    MqttSettings.publish("battery/settings/chargeVoltage", String(_chargeVoltageLimit));
    MqttSettings.publish("battery/settings/chargeCurrentLimitation", String(_chargeCurrentLimit));
    MqttSettings.publish("battery/settings/dischargeVoltageLimitation", String(_dischargeVoltageLimit));

    MqttSettings.publish("battery/stateOfHealth", String(_stateOfHealth));
    if (_chargeCycles != -1) {
        MqttSettings.publish("battery/chargeCycles", String(_chargeCycles));
    }
    if (_balance != -1) {
        MqttSettings.publish("battery/balancingActive", String(_balance ? 1 : 0));
    }
    MqttSettings.publish("battery/temperature", String(_temperature));

    if (_chargedEnergy != -1) {
        MqttSettings.publish("battery/chargedEnergy", String(_chargedEnergy));
    }

    if (_dischargedEnergy != -1) {
        MqttSettings.publish("battery/dischargedEnergy", String(_dischargedEnergy));
    }

    MqttSettings.publish("battery/capacity", String(_totalCapacity));
    MqttSettings.publish("battery/availableCapacity", String(_availableCapacity));

    MqttSettings.publish("battery/CellMinMilliVolt", String(_cellMinMilliVolt));
    MqttSettings.publish("battery/CellMaxMilliVolt", String(_cellMaxMilliVolt));
    MqttSettings.publish("battery/CellDiffMilliVolt", String(_cellMaxMilliVolt - _cellMinMilliVolt));
    MqttSettings.publish("battery/CellMinTemperature", String(_cellMinTemperature));
    MqttSettings.publish("battery/CellMaxTemperature", String(_cellMaxTemperature));
    MqttSettings.publish("battery/CellMinVoltageName", String(_cellMinVoltageName));
    MqttSettings.publish("battery/CellMaxVoltageName", String(_cellMaxVoltageName));
    MqttSettings.publish("battery/CellMinTemperatureName", String(_cellMinTemperatureName));
    MqttSettings.publish("battery/CellMaxTemperatureName", String(_cellMaxTemperatureName));

    MqttSettings.publish("battery/modulesOnline", String(_moduleCountOnline));
    MqttSettings.publish("battery/modulesOffline", String(_moduleCountOffline));
    MqttSettings.publish("battery/modulesBlockingCharge", String(_moduleCountBlockingCharge));
    MqttSettings.publish("battery/modulesBlockingDischarge", String(_moduleCountBlockingDischarge));

    MqttSettings.publish("battery/alarm/overCurrentDischarge", String(_alarmOverCurrentDischarge));
    MqttSettings.publish("battery/alarm/overCurrentCharge", String(_alarmOverCurrentCharge));
    MqttSettings.publish("battery/alarm/underVoltage", String(_alarmUnderVoltage));
    MqttSettings.publish("battery/alarm/overVoltage", String(_alarmOverVoltage));
    MqttSettings.publish("battery/alarm/underTemperature", String(_alarmUnderTemperature));
    MqttSettings.publish("battery/alarm/overTemperature", String(_alarmOverTemperature));
    MqttSettings.publish("battery/alarm/underTemperatureCharge", String(_alarmUnderTemperatureCharge));
    MqttSettings.publish("battery/alarm/overTemperatureCharge", String(_alarmOverTemperatureCharge));
    MqttSettings.publish("battery/alarm/bmsInternal", String(_alarmInternalFailure));
    MqttSettings.publish("battery/alarm/cellImbalance", String(_alarmCellImbalance));

    MqttSettings.publish("battery/warning/highCurrentDischarge", String(_warningHighDischargeCurrent));
    MqttSettings.publish("battery/warning/highCurrentCharge", String(_warningHighChargeCurrent));
    MqttSettings.publish("battery/warning/lowVoltage", String(_warningLowVoltage));
    MqttSettings.publish("battery/warning/highVoltage", String(_warningHighVoltage));
    MqttSettings.publish("battery/warning/lowTemperature", String(_warningLowTemperature));
    MqttSettings.publish("battery/warning/highTemperature", String(_warningHighTemperature));
    MqttSettings.publish("battery/warning/lowTemperatureCharge", String(_warningLowTemperatureCharge));
    MqttSettings.publish("battery/warning/highTemperatureCharge", String(_warningHighTemperatureCharge));
    MqttSettings.publish("battery/warning/bmsInternal", String(_warningInternalFailure));
    MqttSettings.publish("battery/warning/cellImbalance", String(_warningCellImbalance));

    MqttSettings.publish("battery/charging/chargeImmediately", String(_chargeImmediately));
}

void JkBmsBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    using Label = JkBms::DataPointLabel;

    static std::vector<Label> mqttSkip = {
        Label::CellsMilliVolt, // complex data format
        Label::ModificationPassword, // sensitive data
        Label::BatterySoCPercent // already published by base class
        // NOTE that voltage is also published by the base class, however, we
        // previously published it only from here using the respective topic.
        // to avoid a breaking change, we publish the value again using the
        // "old" topic.
    };

    // regularly publish all topics regardless of whether or not their value changed
    bool neverFullyPublished = _lastFullMqttPublish == 0;
    bool intervalElapsed = _lastFullMqttPublish + getMqttFullPublishIntervalMs() < millis();
    bool fullPublish = neverFullyPublished || intervalElapsed;

    for (auto iter = _dataPoints.cbegin(); iter != _dataPoints.cend(); ++iter) {
        // skip data points that did not change since last published
        if (!fullPublish && iter->second.getTimestamp() < _lastMqttPublish) { continue; }

        auto skipMatch = std::find(mqttSkip.begin(), mqttSkip.end(), iter->first);
        if (skipMatch != mqttSkip.end()) { continue; }

        String topic((std::string("battery/") + iter->second.getLabelText()).c_str());
        MqttSettings.publish(topic, iter->second.getValueText().c_str());
    }

    auto oCellVoltages = _dataPoints.get<Label::CellsMilliVolt>();
    if (oCellVoltages.has_value() && (fullPublish || _cellVoltageTimestamp > _lastMqttPublish)) {
        unsigned idx = 1;
        for (auto iter = oCellVoltages->cbegin(); iter != oCellVoltages->cend(); ++iter) {
            String topic("battery/Cell");
            topic += String(idx);
            topic += "MilliVolt";

            MqttSettings.publish(topic, String(iter->second));

            ++idx;
        }

        MqttSettings.publish("battery/CellMinMilliVolt", String(_cellMinMilliVolt));
        MqttSettings.publish("battery/CellAvgMilliVolt", String(_cellAvgMilliVolt));
        MqttSettings.publish("battery/CellMaxMilliVolt", String(_cellMaxMilliVolt));
        MqttSettings.publish("battery/CellDiffMilliVolt", String(_cellMaxMilliVolt - _cellMinMilliVolt));
    }

    auto oAlarms = _dataPoints.get<Label::AlarmsBitmask>();
    if (oAlarms.has_value()) {
        for (auto iter = JkBms::AlarmBitTexts.begin(); iter != JkBms::AlarmBitTexts.end(); ++iter) {
            auto bit = iter->first;
            String value = (*oAlarms & static_cast<uint16_t>(bit))?"1":"0";
            MqttSettings.publish(String("battery/alarms/") + iter->second.data(), value);
        }
    }

    auto oStatus = _dataPoints.get<Label::StatusBitmask>();
    if (oStatus.has_value()) {
        for (auto iter = JkBms::StatusBitTexts.begin(); iter != JkBms::StatusBitTexts.end(); ++iter) {
            auto bit = iter->first;
            String value = (*oStatus & static_cast<uint16_t>(bit))?"1":"0";
            MqttSettings.publish(String("battery/status/") + iter->second.data(), value);
        }
    }

    _lastMqttPublish = millis();
    if (fullPublish) { _lastFullMqttPublish = _lastMqttPublish; }
}

void JkBmsBatteryStats::updateFrom(JkBms::DataPointContainer const& dp)
{
    using Label = JkBms::DataPointLabel;

    setManufacturer("JKBMS");
    auto oProductId = dp.get<Label::ProductId>();
    if (oProductId.has_value()) {
        // the first twelve chars are expected to be the "User Private Data"
        // setting (see smartphone app). the remainder is expected be the BMS
        // name, which can be changed at will using the smartphone app. so
        // there is not always a "JK" in this string. if there is, we still cut
        // the string there to avoid possible regressions.
        setManufacturer(String(oProductId->substr(12).c_str()));
        auto pos = oProductId->rfind("JK");
        if (pos != std::string::npos) {
            setManufacturer(String(oProductId->substr(pos).c_str()));
        }
    }

    auto oSoCValue = dp.get<Label::BatterySoCPercent>();
    if (oSoCValue.has_value()) {
        auto oSoCDataPoint = dp.getDataPointFor<Label::BatterySoCPercent>();
        BatteryStats::setSoC(*oSoCValue, 0/*precision*/,
                oSoCDataPoint->getTimestamp());
    }

    auto oVoltage = dp.get<Label::BatteryVoltageMilliVolt>();
    if (oVoltage.has_value()) {
        auto oVoltageDataPoint = dp.getDataPointFor<Label::BatteryVoltageMilliVolt>();
        BatteryStats::setVoltage(static_cast<float>(*oVoltage) / 1000,
                oVoltageDataPoint->getTimestamp());
    }

    auto oCurrent = dp.get<Label::BatteryCurrentMilliAmps>();
    if (oCurrent.has_value()) {
        auto oCurrentDataPoint = dp.getDataPointFor<Label::BatteryCurrentMilliAmps>();
        BatteryStats::setCurrent(static_cast<float>(*oCurrent) / 1000, 2/*precision*/,
                oCurrentDataPoint->getTimestamp());
    }

    _dataPoints.updateFrom(dp);

    auto oCellVoltages = _dataPoints.get<Label::CellsMilliVolt>();
    if (oCellVoltages.has_value()) {
        for (auto iter = oCellVoltages->cbegin(); iter != oCellVoltages->cend(); ++iter) {
            if (iter == oCellVoltages->cbegin()) {
                _cellMinMilliVolt = _cellAvgMilliVolt = _cellMaxMilliVolt = iter->second;
                continue;
            }
            _cellMinMilliVolt = std::min(_cellMinMilliVolt, iter->second);
            _cellAvgMilliVolt = (_cellAvgMilliVolt + iter->second) / 2;
            _cellMaxMilliVolt = std::max(_cellMaxMilliVolt, iter->second);
        }
        _cellVoltageTimestamp = millis();
    }

    auto oVersion = _dataPoints.get<Label::BmsSoftwareVersion>();
    if (oVersion.has_value()) {
        // raw: "11.XW_S11.262H_"
        //   => Hardware "V11.XW" (displayed in Android app)
        //   => Software "V11.262H" (displayed in Android app)
        auto first = oVersion->find('_');
        if (first != std::string::npos) {
            _hwversion = oVersion->substr(0, first).c_str();

            auto second = oVersion->find('_', first + 1);

            // the 'S' seems to be merely an indicator for "software"?
            if (oVersion->at(first + 1) == 'S') { first++; }

            _fwversion = oVersion->substr(first + 1, second - first - 1).c_str();
        }
    }

    _lastUpdate = millis();
}

void VictronSmartShuntStats::updateFrom(VeDirectShuntController::data_t const& shuntData) {
    BatteryStats::setVoltage(shuntData.batteryVoltage_V_mV / 1000.0, millis());
    BatteryStats::setSoC(static_cast<float>(shuntData.SOC) / 10, 1/*precision*/, millis());
    BatteryStats::setCurrent(static_cast<float>(shuntData.batteryCurrent_I_mA) / 1000, 2/*precision*/, millis());
    _fwversion = shuntData.getFwVersionFormatted();

    _chargeCycles = shuntData.H4;
    _timeToGo = shuntData.TTG / 60;
    _chargedEnergy = static_cast<float>(shuntData.H18) / 100;
    _dischargedEnergy = static_cast<float>(shuntData.H17) / 100;
    setManufacturer(String("Victron ") + shuntData.getPidAsString().data());
    _temperature = shuntData.T;
    _tempPresent = shuntData.tempPresent;
    _midpointVoltage = static_cast<float>(shuntData.VM) / 1000;
    _midpointDeviation = static_cast<float>(shuntData.DM) / 10;
    _instantaneousPower = shuntData.P;
    _consumedAmpHours = static_cast<float>(shuntData.CE) / 1000;
    _lastFullCharge = shuntData.H9 / 60;
    // shuntData.AR is a bitfield, so we need to check each bit individually
    _alarmLowVoltage = shuntData.alarmReason_AR & 1;
    _alarmHighVoltage = shuntData.alarmReason_AR & 2;
    _alarmLowSOC = shuntData.alarmReason_AR & 4;
    _alarmLowTemperature = shuntData.alarmReason_AR & 32;
    _alarmHighTemperature = shuntData.alarmReason_AR & 64;

    _lastUpdate = VeDirectShunt.getLastUpdate();
}

void VictronSmartShuntStats::getLiveViewData(JsonVariant& root) const {
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeCycles", _chargeCycles, "", 0);
    addLiveViewValue(root, "chargedEnergy", _chargedEnergy, "kWh", 2);
    addLiveViewValue(root, "dischargedEnergy", _dischargedEnergy, "kWh", 2);
    addLiveViewValue(root, "instantaneousPower", _instantaneousPower, "W", 0);
    addLiveViewValue(root, "consumedAmpHours", _consumedAmpHours, "Ah", 3);
    addLiveViewValue(root, "midpointVoltage", _midpointVoltage, "V", 2);
    addLiveViewValue(root, "midpointDeviation", _midpointDeviation, "%", 1);
    addLiveViewValue(root, "lastFullCharge", _lastFullCharge, "min", 0);
    if (_tempPresent) {
        addLiveViewValue(root, "temperature", _temperature, "°C", 0);
    }

    addLiveViewAlarm(root, "lowVoltage", _alarmLowVoltage);
    addLiveViewAlarm(root, "highVoltage", _alarmHighVoltage);
    addLiveViewAlarm(root, "lowSOC", _alarmLowSOC);
    addLiveViewAlarm(root, "lowTemperature", _alarmLowTemperature);
    addLiveViewAlarm(root, "highTemperature", _alarmHighTemperature);
}

void VictronSmartShuntStats::mqttPublish() const {
    BatteryStats::mqttPublish();

    MqttSettings.publish("battery/chargeCycles", String(_chargeCycles));
    MqttSettings.publish("battery/chargedEnergy", String(_chargedEnergy));
    MqttSettings.publish("battery/dischargedEnergy", String(_dischargedEnergy));
    MqttSettings.publish("battery/instantaneousPower", String(_instantaneousPower));
    MqttSettings.publish("battery/consumedAmpHours", String(_consumedAmpHours));
    MqttSettings.publish("battery/lastFullCharge", String(_lastFullCharge));
    MqttSettings.publish("battery/midpointVoltage", String(_midpointVoltage));
    MqttSettings.publish("battery/midpointDeviation", String(_midpointDeviation));
}

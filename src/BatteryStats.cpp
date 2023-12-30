// SPDX-License-Identifier: GPL-2.0-or-later
#include <vector>
#include <algorithm>
#include "BatteryStats.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "JkBmsDataPoints.h"

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
    std::string const& section, std::string const& name, std::string const& text)
{
    root["values"][section][name] = text;
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

void BatteryStats::getLiveViewData(JsonVariant& root) const
{
    root[F("manufacturer")] = _manufacturer;
    root[F("data_age")] = getAgeSeconds();

    addLiveViewValue(root, "SoC", _SoC, "%", 0);
}

void PylontechBatteryStats::getLiveViewData(JsonVariant& root) const
{
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltage, "V", 1);
    addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "dischargeCurrentLimitation", _dischargeCurrentLimitation, "A", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
    addLiveViewValue(root, "voltage", _voltage, "V", 2);
    addLiveViewValue(root, "current", _current, "A", 1);
    addLiveViewValue(root, "temperature", _temperature, "°C", 1);

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

void JkBmsBatteryStats::getJsonData(JsonVariant& root, bool verbose) const
{
    BatteryStats::getLiveViewData(root);

    using Label = JkBms::DataPointLabel;

    auto oVoltage = _dataPoints.get<Label::BatteryVoltageMilliVolt>();
    if (oVoltage.has_value()) {
        addLiveViewValue(root, "voltage",
                static_cast<float>(*oVoltage) / 1000, "V", 2);
    }

    auto oCurrent = _dataPoints.get<Label::BatteryCurrentMilliAmps>();
    if (oCurrent.has_value()) {
        addLiveViewValue(root, "current",
                static_cast<float>(*oCurrent) / 1000, "A", 2);
    }

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

void BatteryStats::mqttPublish() const
{
    MqttSettings.publish(F("battery/manufacturer"), _manufacturer);
    MqttSettings.publish(F("battery/dataAge"), String(getAgeSeconds()));
    MqttSettings.publish(F("battery/stateOfCharge"), String(_SoC));
}

void PylontechBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    MqttSettings.publish(F("battery/settings/chargeVoltage"), String(_chargeVoltage));
    MqttSettings.publish(F("battery/settings/chargeCurrentLimitation"), String(_chargeCurrentLimitation));
    MqttSettings.publish(F("battery/settings/dischargeCurrentLimitation"), String(_dischargeCurrentLimitation));
    MqttSettings.publish(F("battery/stateOfHealth"), String(_stateOfHealth));
    MqttSettings.publish(F("battery/voltage"), String(_voltage));
    MqttSettings.publish(F("battery/current"), String(_current));
    MqttSettings.publish(F("battery/temperature"), String(_temperature));
    MqttSettings.publish(F("battery/alarm/overCurrentDischarge"), String(_alarmOverCurrentDischarge));
    MqttSettings.publish(F("battery/alarm/overCurrentCharge"), String(_alarmOverCurrentCharge));
    MqttSettings.publish(F("battery/alarm/underTemperature"), String(_alarmUnderTemperature));
    MqttSettings.publish(F("battery/alarm/overTemperature"), String(_alarmOverTemperature));
    MqttSettings.publish(F("battery/alarm/underVoltage"), String(_alarmUnderVoltage));
    MqttSettings.publish(F("battery/alarm/overVoltage"), String(_alarmOverVoltage));
    MqttSettings.publish(F("battery/alarm/bmsInternal"), String(_alarmBmsInternal));
    MqttSettings.publish(F("battery/warning/highCurrentDischarge"), String(_warningHighCurrentDischarge));
    MqttSettings.publish(F("battery/warning/highCurrentCharge"), String(_warningHighCurrentCharge));
    MqttSettings.publish(F("battery/warning/lowTemperature"), String(_warningLowTemperature));
    MqttSettings.publish(F("battery/warning/highTemperature"), String(_warningHighTemperature));
    MqttSettings.publish(F("battery/warning/lowVoltage"), String(_warningLowVoltage));
    MqttSettings.publish(F("battery/warning/highVoltage"), String(_warningHighVoltage));
    MqttSettings.publish(F("battery/warning/bmsInternal"), String(_warningBmsInternal));
    MqttSettings.publish(F("battery/charging/chargeEnabled"), String(_chargeEnabled));
    MqttSettings.publish(F("battery/charging/dischargeEnabled"), String(_dischargeEnabled));
    MqttSettings.publish(F("battery/charging/chargeImmediately"), String(_chargeImmediately));
}

void JkBmsBatteryStats::mqttPublish() const
{
    BatteryStats::mqttPublish();

    using Label = JkBms::DataPointLabel;

    static std::vector<Label> mqttSkip = {
        Label::CellsMilliVolt, // complex data format
        Label::ModificationPassword, // sensitive data
        Label::BatterySoCPercent // already published by base class
    };

    CONFIG_T& config = Configuration.get();

    // publish all topics every minute, unless the retain flag is enabled
    bool fullPublish = _lastFullMqttPublish + 60 * 1000 < millis();
    fullPublish &= !config.Mqtt.Retain;

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
            MqttSettings.publish(String("battery/alarms/") + iter->second.c_str(), value);
        }
    }

    auto oStatus = _dataPoints.get<Label::StatusBitmask>();
    if (oStatus.has_value()) {
        for (auto iter = JkBms::StatusBitTexts.begin(); iter != JkBms::StatusBitTexts.end(); ++iter) {
            auto bit = iter->first;
            String value = (*oStatus & static_cast<uint16_t>(bit))?"1":"0";
            MqttSettings.publish(String("battery/status/") + iter->second.c_str(), value);
        }
    }

    _lastMqttPublish = millis();
    if (fullPublish) { _lastFullMqttPublish = _lastMqttPublish; }
}

void JkBmsBatteryStats::updateFrom(JkBms::DataPointContainer const& dp)
{
    using Label = JkBms::DataPointLabel;

    _manufacturer = "JKBMS";
    auto oProductId = dp.get<Label::ProductId>();
    if (oProductId.has_value()) {
        _manufacturer = oProductId->c_str();
        auto pos = oProductId->rfind("JK");
        if (pos != std::string::npos) {
            _manufacturer = oProductId->substr(pos).c_str();
        }
    }

    auto oSoCValue = dp.get<Label::BatterySoCPercent>();
    if (oSoCValue.has_value()) {
        _SoC = *oSoCValue;
        auto oSoCDataPoint = dp.getDataPointFor<Label::BatterySoCPercent>();
        _lastUpdateSoC = oSoCDataPoint->getTimestamp();
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

    _lastUpdate = millis();
}

void VictronSmartShuntStats::updateFrom(VeDirectShuntController::veShuntStruct const& shuntData) {
    _SoC = shuntData.SOC / 10;
    _voltage = shuntData.V;
    _current = shuntData.I;
    _modelName = shuntData.getPidAsString();
    _chargeCycles = shuntData.H4;
    _timeToGo = shuntData.TTG / 60;
    _chargedEnergy = shuntData.H18 / 100;
    _dischargedEnergy = shuntData.H17 / 100;
    _manufacturer = "Victron " + _modelName;
    _temperature = shuntData.T;
    _tempPresent = shuntData.tempPresent;

    // shuntData.AR is a bitfield, so we need to check each bit individually
    _alarmLowVoltage = shuntData.AR & 1;
    _alarmHighVoltage = shuntData.AR & 2;
    _alarmLowSOC = shuntData.AR & 4;
    _alarmLowTemperature = shuntData.AR & 32;
    _alarmHighTemperature = shuntData.AR & 64;

    _lastUpdate = VeDirectShunt.getLastUpdate();
    _lastUpdateSoC = VeDirectShunt.getLastUpdate();
}

void VictronSmartShuntStats::getLiveViewData(JsonVariant& root) const {
    BatteryStats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "voltage", _voltage, "V", 2);
    addLiveViewValue(root, "current", _current, "A", 1);
    addLiveViewValue(root, "chargeCycles", _chargeCycles, "", 0);
    addLiveViewValue(root, "chargedEnergy", _chargedEnergy, "KWh", 1);
    addLiveViewValue(root, "dischargedEnergy", _dischargedEnergy, "KWh", 1);
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

    MqttSettings.publish(F("battery/voltage"), String(_voltage));
    MqttSettings.publish(F("battery/current"), String(_current));
    MqttSettings.publish(F("battery/chargeCycles"), String(_chargeCycles));
    MqttSettings.publish(F("battery/chargedEnergy"), String(_chargedEnergy));
    MqttSettings.publish(F("battery/dischargedEnergy"), String(_dischargedEnergy));
}

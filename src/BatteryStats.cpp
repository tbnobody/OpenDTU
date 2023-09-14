// SPDX-License-Identifier: GPL-2.0-or-later
#include "BatteryStats.h"
#include "MqttSettings.h"
#include "JkBmsDataPoints.h"

template<typename T>
void BatteryStats::addLiveViewValue(JsonVariant& root, std::string const& name,
    T&& value, std::string const& unit, uint8_t precision) const
{
    auto jsonValue = root["values"][name];
    jsonValue["v"] = value;
    jsonValue["u"] = unit;
    jsonValue["d"] = precision;
}

void BatteryStats::addLiveViewText(JsonVariant& root, std::string const& name,
    std::string const& text) const
{
    root["values"][name] = text;
}

void BatteryStats::addLiveViewWarning(JsonVariant& root, std::string const& name,
    bool warning) const
{
    if (!warning) { return; }
    root["issues"][name] = 1;
}

void BatteryStats::addLiveViewAlarm(JsonVariant& root, std::string const& name,
    bool alarm) const
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

    addLiveViewText(root, "chargeEnabled", (_chargeEnabled?"yes":"no"));
    addLiveViewText(root, "dischargeEnabled", (_dischargeEnabled?"yes":"no"));
    addLiveViewText(root, "chargeImmediately", (_chargeImmediately?"yes":"no"));

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

void JkBmsBatteryStats::getLiveViewData(JsonVariant& root) const
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

    auto oTemperature = _dataPoints.get<Label::BatteryTempOneCelsius>();
    if (oTemperature.has_value()) {
        addLiveViewValue(root, "temperature", *oTemperature, "°C", 0);
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
}

void JkBmsBatteryStats::updateFrom(JkBms::DataPointContainer const& dp)
{
    _dataPoints.updateFrom(dp);

    using Label = JkBms::DataPointLabel;

    _manufacturer = "JKBMS";
    auto oProductId = _dataPoints.get<Label::ProductId>();
    if (oProductId.has_value()) {
        _manufacturer = oProductId->c_str();
        auto pos = oProductId->rfind("JK");
        if (pos != std::string::npos) {
            _manufacturer = oProductId->substr(pos).c_str();
        }
    }

    auto oSoCValue = _dataPoints.get<Label::BatterySoCPercent>();
    if (oSoCValue.has_value()) {
        _SoC = *oSoCValue;
        auto oSoCDataPoint = _dataPoints.getDataPointFor<Label::BatterySoCPercent>();
        _lastUpdateSoC = oSoCDataPoint->getTimestamp();
    }

    _lastUpdate = millis();
}

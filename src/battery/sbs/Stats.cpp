// SPDX-License-Identifier: GPL-2.0-or-later
#include <MqttSettings.h>
#include <battery/sbs/Stats.h>

namespace Batteries::SBS {

void Stats::getLiveViewData(JsonVariant& root) const
{
    ::Batteries::Stats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltage, "V", 1);
    addLiveViewValue(root, "stateOfHealth", _stateOfHealth, "%", 0);
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

void Stats::mqttPublish() const
{
    ::Batteries::Stats::mqttPublish();

    MqttSettings.publish("battery/settings/chargeVoltage", String(_chargeVoltage));
    MqttSettings.publish("battery/stateOfHealth", String(_stateOfHealth));
    MqttSettings.publish("battery/temperature", String(_temperature));
    MqttSettings.publish("battery/alarm/underVoltage", String(_alarmUnderVoltage));
    MqttSettings.publish("battery/alarm/overVoltage", String(_alarmOverVoltage));
    MqttSettings.publish("battery/alarm/bmsInternal", String(_alarmBmsInternal));
    MqttSettings.publish("battery/warning/highCurrentDischarge", String(_warningHighCurrentDischarge));
    MqttSettings.publish("battery/warning/highCurrentCharge", String(_warningHighCurrentCharge));
    MqttSettings.publish("battery/charging/chargeEnabled", String(_chargeEnabled));
    MqttSettings.publish("battery/charging/dischargeEnabled", String(_dischargeEnabled));
}

} // namespace Batteries::SBS

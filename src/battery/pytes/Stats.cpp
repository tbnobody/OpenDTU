// SPDX-License-Identifier: GPL-2.0-or-later
#include <MqttSettings.h>
#include <battery/pytes/Stats.h>

namespace Batteries::Pytes {

void Stats::getLiveViewData(JsonVariant& root) const
{
    ::Batteries::Stats::getLiveViewData(root);

    // values go into the "Status" card of the web application
    addLiveViewValue(root, "chargeVoltage", _chargeVoltageLimit, "V", 1);
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

void Stats::mqttPublish() const
{
    ::Batteries::Stats::mqttPublish();

    MqttSettings.publish("battery/settings/chargeVoltage", String(_chargeVoltageLimit));
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

} // namespace Batteries::Pytes

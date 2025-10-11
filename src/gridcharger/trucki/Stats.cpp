// SPDX-License-Identifier: GPL-2.0-or-later
#include <Arduino.h>
#include <Configuration.h>
#include <MqttSettings.h>
#include <gridcharger/trucki/Stats.h>

namespace GridChargers::Trucki {

uint32_t Stats::getLastUpdate() const
{
    return _dataPoints.getLastUpdate();
}

std::optional<float> Stats::getInputPower() const
{
   return _dataPoints.get<DataPointLabel::AcPower>();
}

void Stats::updateFrom(DataPointContainer const& dataPoints) const
{
    _dataPoints.updateFrom(dataPoints);
}

void Stats::getLiveViewData(JsonVariant& root) const
{
    root["vendorName"] = "Trucki";
    root["productName"] = "T2HG/T2MG";
    root["provider"] = GridChargerProviderType::TRUCKI;

    const auto dataAge = millis() - getLastUpdate();
    root["dataAge"] = dataAge;
    root["reachable"] = dataAge < 10000;

    const auto oDcPower = _dataPoints.get<DataPointLabel::DcPower>();
    const auto oDcCurrent = _dataPoints.get<DataPointLabel::DcCurrent>();
    root["producing"] = oDcPower.value_or(0.0f) > 10.0f && oDcCurrent.value_or(0.0f) > 0.1f;

    addStringInSection<DataPointLabel::ZEPC>(root, "device", "trucki.zepc", false);
    addStringInSection<DataPointLabel::State>(root, "device", "state", false);
    addStringInSection<DataPointLabel::BatteryGridState>(root, "device", "trucki.batteryGridState");
    addValueInSection<DataPointLabel::Temperature>(root, "device", "temp", 1);
    addValueInSection<DataPointLabel::Efficiency>(root, "device", "efficiency", 0);
    addValueInSection<DataPointLabel::DayEnergy>(root, "device", "yieldDay", 2);
    addValueInSection<DataPointLabel::TotalEnergy>(root, "device", "yieldTotal", 2);

    addValueInSection<DataPointLabel::AcVoltage>(root, "input", "voltage", 1);
    addValueInSection<DataPointLabel::AcPower>(root, "input", "power", 1);
    addValueInSection<DataPointLabel::AcPowerSetpoint>(root, "input", "powerSetpoint", 1);
    addValueInSection<DataPointLabel::MinAcPower>(root, "input", "minPower", 0);
    addValueInSection<DataPointLabel::MaxAcPower>(root, "input", "maxPower", 0);

    addValueInSection<DataPointLabel::DcVoltage>(root, "output", "voltage", 2);
    addValueInSection<DataPointLabel::DcVoltageSetpoint>(root, "output", "voltageSetpoint", 2);
    addValueInSection<DataPointLabel::DcPower>(root, "output", "power", 1);
    addValueInSection<DataPointLabel::DcCurrent>(root, "output", "current", 2);

    addValueInSection<DataPointLabel::DcVoltageOffline>(root, "settings", "offlineVoltage", 0);
    addValueInSection<DataPointLabel::DcCurrentOffline>(root, "settings", "offlineCurrent", 0);
}

}; // namespace GridChargers::Trucki

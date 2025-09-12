// SPDX-License-Identifier: GPL-2.0-or-later
#include <battery/zendure/Constants.h>
#include <battery/zendure/HassIntegration.h>

namespace Batteries::Zendure {

 HassIntegration::HassIntegration(std::shared_ptr<Stats> spStats)
    : ::Batteries::HassIntegration(spStats) { }

void HassIntegration::publishSensors() const
{
    ::Batteries::HassIntegration::publishSensors();

    publishSensor("Cell Min Voltage", NULL, "cellMinMilliVolt", "voltage", "measurement", "mV");
    publishSensor("Cell Average Voltage", NULL, "cellAvgMilliVolt", "voltage", "measurement", "mV");
    publishSensor("Cell Max Voltage", NULL, "cellMaxMilliVolt", "voltage", "measurement", "mV");
    publishSensor("Cell Voltage Diff", "mdi:battery-alert", "cellDiffMilliVolt", "voltage", "measurement", "mV");
    publishSensor("Cell Max Temperature", NULL, "cellMaxTemperature", "temperature", "measurement", "°C");
    publishSensor("Charge Power", "mdi:battery-charging", "chargePower", "power", "measurement", "W");
    publishSensor("Discharge Power", "mdi:battery-discharging", "dischargePower", "power", "measurement", "W");
    publishBinarySensor("Battery Heating", NULL, "heating", "1", "0");
    publishSensor("State", NULL, "state");
    publishSensor("Number of Batterie Packs", "mdi:counter", "numPacks");
    publishSensor("Efficiency", NULL, "efficiency", NULL, "measurement", "%");
    publishSensor("Last Full Charge", "mdi:timelapse", "lastFullCharge", NULL, NULL, "h");
    publishSensor("Last Empty", "mdi:timelapse", "lastEmpty", NULL, NULL, "h");
    publishSensor("Charge Through State", NULL, "chargeThroughState");

    publishSensor("Solar Power MPPT 1", "mdi:solar-power", "solarPowerMppt1", "power", "measurement", "W");
    publishSensor("Solar Power MPPT 2", "mdi:solar-power", "solarPowerMppt2", "power", "measurement", "W");
    publishSensor("Total Output Power", NULL, "outputPower", "power", "measurement", "W");
    publishSensor("Total Input Power", NULL, "inputPower", "power", "measurement", "W");
    publishBinarySensor("Bypass State", NULL, "bypass", "1", "0");

    publishSensor("Control Mode", NULL, "controlMode", "settings");
    publishSensor("Output Power Limit", NULL, "settings/outputLimitPower", "power", "settings", "W");
    publishSensor("Input Power Limit", NULL, "settings/inputLimitPower", "power", "settings", "W");
    publishSensor("Minimum State of Charge", NULL, "settings/stateOfChargeMin", NULL, "settings", "%");
    publishSensor("Maximum State of Charge", NULL, "settings/stateOfChargeMax", NULL, "settings", "%");
    publishSensor("Bypass Mode", NULL, "settings/bypassMode", "settings");

    for (size_t i = 1 ; i <= ZENDURE_MAX_PACKS ; i++) {
        const auto id = String(i);
        const auto bat = String("Pack#" + id + ": ");
        publishSensor(String(bat + "Cell Min Voltage").c_str(), NULL, String(id + "/cellMinMilliVolt").c_str(), "voltage", "measurement", "mV", false);
        publishSensor(String(bat + "Cell Average Voltage").c_str(), NULL, String(id + "/cellAvgMilliVolt").c_str(), "voltage", "measurement", "mV", false);
        publishSensor(String(bat + "Cell Max Voltage").c_str(), NULL, String(id + "/cellMaxMilliVolt").c_str(), "voltage", "measurement", "mV", false);
        publishSensor(String(bat + "Cell Voltage Diff").c_str(), "mdi:battery-alert", String(id + "/cellDiffMilliVolt").c_str(), "voltage", "measurement", "mV", false);
        publishSensor(String(bat + "Cell Max Temperature").c_str(), NULL, String(id + "/cellMaxTemperature").c_str(), "temperature", "measurement", "°C", false);
        publishSensor(String(bat + "Power").c_str(), NULL, String(id + "/power").c_str(), "power", "measurement", "W", false);
        publishSensor(String(bat + "Voltage").c_str(), NULL, String(id + "/voltage").c_str(), "voltage", "measurement", "V", false);
        publishSensor(String(bat + "Current").c_str(), NULL, String(id + "/current").c_str(), "current", "measurement", "A", false);
        publishSensor(String(bat + "State Of Charge").c_str(), NULL, String(id + "/stateOfCharge").c_str(), NULL, "measurement", "%", false);
        publishSensor(String(bat + "State Of Health").c_str(), NULL, String(id + "/stateOfHealth").c_str(), NULL, "measurement", "%", false);
        publishSensor(String(bat + "State").c_str(), NULL, String(id + "/state").c_str(), NULL, NULL, NULL, false);
    }
}

} // namespace Batteries::Zendure

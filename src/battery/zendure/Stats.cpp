// SPDX-License-Identifier: GPL-2.0-or-later
#include <MqttSettings.h>
#include <battery/zendure/Stats.h>
#include <Utils.h>

namespace Batteries::Zendure {

void Stats::getLiveViewData(JsonVariant& root) const
{
    ::Batteries::Stats::getLiveViewData(root);

    auto const& config = Configuration.get();

    // values go into the "Status" card of the web application
    std::string section("status");
    addLiveViewInSection(root, section, "totalInputPower", _input_power, "W", 0);
    addLiveViewInSection(root, section, "chargePower", _charge_power, "W", 0);
    addLiveViewInSection(root, section, "dischargePower", _discharge_power, "W", 0);
    addLiveViewInSection(root, section, "totalOutputPower", _output_power, "W", 0);
    addLiveViewInSection(root, section, "efficiency", _efficiency, "%", 3);
    addLiveViewInSection(root, section, "batteries", _num_batteries, "", 0);
    addLiveViewInSection(root, section, "capacity", _capacity, "Wh", 0);
    addLiveViewInSection(root, section, "availableCapacity", _capacity_avail, "Wh", 0);
    addLiveViewInSection(root, section, "useableCapacity", getUseableCapacity(), "Wh", 0);
    addLiveViewTextInSection(root, section, "state", std::string(stateToString(_state)));
    addLiveViewBooleanInSection(root, section, "heatState", _heat_state);
    addLiveViewBooleanInSection(root, section, "bypassState", _bypass_state);
    addLiveViewTextInSection(root, section, "zendure.chargeThroughState", "zendure.chargeThroughStates." + std::string(chargeThroughStateToString(_charge_through_state)));
    addLiveViewInSection(root, section, "lastFullCharge", _last_full_hours, "h", 0);
    addLiveViewInSection(root, section, "lastEmptyCharge", _last_empty_hours, "h", 0);
    addLiveViewInSection(root, section, "remainOutTime", _remain_out_time, "min", 0);
    addLiveViewInSection(root, section, "remainInTime", _remain_in_time, "min", 0);

    // values go into the "Settings" card of the web application
    section = "settings";
    addLiveViewTextInSection(root, section, "controlMode", std::string(controlModeToString(config.Battery.Zendure.ControlMode)));
    addLiveViewInSection(root, section, "maxInversePower", _inverse_max, "W", 0);
    addLiveViewInSection(root, section, "outputLimit", _output_limit, "W", 0);
    addLiveViewInSection(root, section, "inputLimit", _output_limit, "W", 0);
    addLiveViewInSection(root, section, "minSoC", _soc_min, "%", 1);
    addLiveViewInSection(root, section, "maxSoC", _soc_max, "%", 1);
    addLiveViewBooleanInSection(root, section, "autoRecover", _auto_recover);
    addLiveViewBooleanInSection(root, section, "autoShutdown", _auto_shutdown);
    addLiveViewTextInSection(root, section, "bypassMode", std::string(bypassModeToString(_bypass_mode)));
    addLiveViewBooleanInSection(root, section, "buzzer", _buzzer);

    // values go into the "Solar Panels" card of the web application
    section = "panels";
    addLiveViewInSection(root, section, "solarInputPower1", _solar_power_1, "W", 0);
    addLiveViewInSection(root, section, "solarInputPower2", _solar_power_2, "W", 0);

    // pack data goes to dedicated cards of the web application
    char buff[30];
    for (const auto& [index, value] : _packData) {
        snprintf(buff, sizeof(buff), "_%s [%s]", value->getName().c_str(), value->getSerial().c_str());
        section = std::string(buff);
        addLiveViewTextInSection(root, section, "state", std::string(stateToString(value->_state)));
        addLiveViewInSection(root, section, "cellMinVoltage", value->_cell_voltage_min, "mV", 0);
        addLiveViewInSection(root, section, "cellAvgVoltage", value->_cell_voltage_avg, "mV", 0);
        addLiveViewInSection(root, section, "cellMaxVoltage", value->_cell_voltage_max, "mV", 0);
        addLiveViewInSection(root, section, "cellDiffVoltage", value->_cell_voltage_spread, "mV", 0);
        addLiveViewInSection(root, section, "cellMaxTemperature", value->_cell_temperature_max, "°C", 1);
        addLiveViewInSection(root, section, "voltage", value->_voltage_total, "V", 2);
        addLiveViewInSection(root, section, "power", value->_power, "W", 0);
        addLiveViewInSection(root, section, "current", value->_current, "A", 2);
        addLiveViewInSection(root, section, "SoC", value->_soc_level, "%", 1);
        addLiveViewInSection(root, section, "stateOfHealth", value->_state_of_health, "%", 1, true);
        addLiveViewInSection(root, section, "capacity", value->_capacity, "Wh", 0);
        addLiveViewInSection(root, section, "availableCapacity", value->_capacity_avail, "Wh", 0);
        addLiveViewTextInSection(root, section, "FwVersion", std::string(value->_fwversion.c_str()), false);
    }
}

void Stats::mqttPublish() const
{
    ::Batteries::Stats::mqttPublish();

    auto const& config = Configuration.get();

    publish("battery/cellMinMilliVolt", _cellMinMilliVolt);
    publish("battery/cellAvgMilliVolt", _cellAvgMilliVolt);
    publish("battery/cellMaxMilliVolt", _cellMaxMilliVolt);
    publish("battery/cellDiffMilliVolt", _cellDeltaMilliVolt);
    publish("battery/cellMaxTemperature", _cellTemperature);
    publish("battery/chargePower", _charge_power);
    publish("battery/dischargePower", _discharge_power);
    publish("battery/heating", String(static_cast<uint8_t>(_heat_state)));
    publish("battery/state", String(stateToString(_state)));
    publish("battery/numPacks", _num_batteries);
    publish("battery/efficiency", _efficiency);
    publish("battery/serial", _serial);

    for (const auto& [index, value] : _packData) {
        auto id = String(index);
        publish("battery/" + id + "/cellMinMilliVolt", value->_cell_voltage_min);
        publish("battery/" + id + "/cellMaxMilliVolt", value->_cell_voltage_max);
        publish("battery/" + id + "/cellDiffMilliVolt", value->_cell_voltage_spread);
        publish("battery/" + id + "/cellAvgMilliVolt", value->_cell_voltage_avg);
        publish("battery/" + id + "/cellMaxTemperature", value->_cell_temperature_max);
        publish("battery/" + id + "/voltage", value->_voltage_total);
        publish("battery/" + id + "/power", value->_power);
        publish("battery/" + id + "/current", value->_current);
        publish("battery/" + id + "/stateOfCharge", value->_soc_level, 1);
        publish("battery/" + id + "/stateOfHealth", value->_state_of_health, 1);
        publish("battery/" + id + "/state", String(stateToString(value->_state)));
        publish("battery/" + id + "/serial", value->getSerial());
        publish("battery/" + id + "/name", value->getName());
        publish("battery/" + id + "/capacity", value->_capacity);
    }

    publish("battery/solarPowerMppt1", _solar_power_1);
    publish("battery/solarPowerMppt2", _solar_power_2);
    publish("battery/outputPower", _output_power);
    publish("battery/inputPower", _input_power);
    publish("battery/bypass", static_cast<uint8_t>(_bypass_state));
    publish("battery/lastFullCharge", _last_full_hours);
    publish("battery/lastEmpty", _last_empty_hours);

    publish("battery/chargeThroughState", String(chargeThroughStateToString(_charge_through_state)));

    publish("battery/settings/controlMode", String(controlModeToString(config.Battery.Zendure.ControlMode)));
    publish("battery/settings/outputLimitPower", _output_limit);
    publish("battery/settings/inputLimitPower", _input_limit);
    publish("battery/settings/stateOfChargeMin", _soc_min, 1);
    publish("battery/settings/stateOfChargeMax", _soc_max, 1);
    publish("battery/settings/bypassMode", String(bypassModeToString(_bypass_mode)));
}

std::shared_ptr<PackStats> Stats::getPackData(size_t index) const {
    try
    {
        return _packData.at(index);
    }
    catch(const std::out_of_range& ex)
    {
        return nullptr;
    }
}

std::shared_ptr<PackStats> Stats::addPackData(size_t index, String serial) {
    std::shared_ptr<PackStats> pack;
    try
    {
        pack = _packData.at(index);
        pack->setSerial(serial);
    }
    catch(const std::out_of_range& ex)
    {
        pack = PackStats::fromSerial(serial);

        if (pack != nullptr) {
            _packData[index] = pack;
        }
    }
    return pack;
}

} // namespace Batteries::Zendure

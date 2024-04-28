// SPDX-License-Identifier: GPL-2.0-or-later
#include "JsyMk.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "PinMapping.h"

JsyMkClass JsyMk;

namespace {
// HA status classes
constexpr std::string_view scMeasurement("measurement");
constexpr std::string_view scTotalIncreasing("total_increasing");

// Name, unit, digits, HA device class, HA status class
constexpr std::array<std::tuple<std::string_view, std::string_view, size_t, std::string_view, std::string_view>, 16> fieldInfos = { //
    { { "Address", {}, {}, {}, {} },
        { "Manufacturer", {}, {}, {}, {} },
        { "Model", {}, {}, {}, {} },
        { "Version", {}, {}, {}, {} },
        { "Voltage Range", "V", 0, "voltage", scMeasurement },
        { "Current Range", "A", 0, "current", scMeasurement },
        { "Voltage", "V", 2, "voltage", scMeasurement },
        { "Current", "A", 2, "current", scMeasurement },
        { "Power", "W", 0, "power", scMeasurement },
        { "Power Factor", "%", 2, "power_factor", scMeasurement },
        { "Frequency", "Hz", 2, "frequency", scMeasurement },
        { "Negative", {}, {}, {}, scMeasurement },
        { "Positive Energy", "kWh", 2, "energy", scTotalIncreasing },
        { "Negative Energy", "kWh", 2, "energy", scTotalIncreasing },
        { "Today Positive Energy", "kWh", 2, "energy", scTotalIncreasing },
        { "Today Negative Energy", "kWh", 2, "energy", scTotalIncreasing } }
};
}
JsyMkClass::JsyMkClass()
    : _jsymk(1)
    , _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&JsyMkClass::loop, this))
{
}

void JsyMkClass::init(Scheduler& scheduler)
{
    const CONFIG_T& config = Configuration.get();
    const PinMapping_t& pin = PinMapping.get();

    if (PinMapping.isValidSerialModbusConfig()) {
        // Initialize inverter communication
        _jsymk.setUpdatedCallback(
            [&] {
                _lastUpdate = millis();
            });
        _jsymk.setErrorCallback([&](SerialModbusRTU::ec error_code) {
            MessageOutput.printf("JSY-MK error %d\n", static_cast<uint8_t>(error_code));
        });

        _jsymk.setUpdatePeriod(std::chrono::seconds(config.SerialModbus.PollInterval));
        _jsymk.begin(config.SerialModbus.BaudRate, SERIAL_8N1, pin.serial_modbus_rx, pin.serial_modbus_tx);

        scheduler.addTask(_loopTask);
        _loopTask.enable();
    }
}

void JsyMkClass::loop()
{
    _jsymk.loop();

    if (!_initialised && _jsymk.getModel() != 0) {
        _initialised = true;

        MessageOutput.println("JSY-MK-xxxT Initialized");
        MessageOutput.printf("Model: %s %s\n", _jsymk.getModelAsString().c_str(), _jsymk.getVersionAsString().c_str());
        MessageOutput.printf("Ranges: %dV %dA\n", _jsymk.getVoltageRange(), _jsymk.getCurrentRange());
    } else {
        // Check current time
        time_t now = time(nullptr);
        const auto* lt = localtime(&now);

        if (lt->tm_hour == 0 && lt->tm_min == 0 && lt->tm_sec <= Configuration.get().SerialModbus.PollInterval) {
            _todayPositiveRef = 0;
            _todayNegativeRef = 0;
        }

        if (_todayPositiveRef == 0) {
            _todayPositiveRef = _jsymk.getPositiveEnergy();
        }
        if (_todayNegativeRef == 0) {
            _todayNegativeRef = _jsymk.getNegativeEnergy();
        }
    }
}

bool JsyMkClass::isInitialised() const
{
    return _initialised;
}

uint32_t JsyMkClass::getLastUpdate() const
{
    return _lastUpdate;
}

uint32_t JsyMkClass::getPollInterval() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(_jsymk.getUpdatePeriod()).count();
}

void JsyMkClass::setPollInterval(const uint32_t interval)
{
    _jsymk.setUpdatePeriod(std::chrono::seconds(interval));
}

size_t JsyMkClass::getChannelNumber() const
{
    return _jsymk.getChannelNumber();
}

String JsyMkClass::getFieldName(size_t /*channel*/, Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    const auto& fieldName = std::get<0>(fieldInfos[static_cast<size_t>(fieldId)]);
    return String(fieldName.data(), fieldName.size());
}

String JsyMkClass::getFieldString(size_t channel, Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    switch (fieldId) {
    case Field_t::ADDRESS:
        return String(_jsymk.getAddress());

    case Field_t::MANUFACTURER:
        return String(_jsymk.getManufacturer().c_str());

    case Field_t::MODEL:
        return String(_jsymk.getModelAsString().c_str());

    case Field_t::VERSION:
        return String(_jsymk.getVersionAsString().c_str());

    default:
        break;
    }

    return String(getFieldValue(channel, fieldId), getFieldDigits(fieldId));
}

float JsyMkClass::getFieldValue(size_t channel, Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return 0;

    const CONFIG_T& config = Configuration.get();
    auto isLogicalNegative = [&]() -> bool {
        return config.PowerMeter.channel[channel].InvertDirection ? !_jsymk.isNegative() : _jsymk.isNegative();
    };

    switch (fieldId) {
    case Field_t::VOLTAGE_RANGE:
        return static_cast<float>(_jsymk.getVoltageRange());

    case Field_t::CURRENT_RANGE:
        return static_cast<float>(_jsymk.getCurrentRange());

    case Field_t::VOLTAGE:
        return _jsymk.getVoltage();

    case Field_t::CURRENT:
        return _jsymk.getCurrent();

    case Field_t::POWER:
        if (config.PowerMeter.channel[channel].NegativePower) {
            return (isLogicalNegative() ? -_jsymk.getPower() : _jsymk.getPower());
        }
        return _jsymk.getPower();

    case Field_t::POWER_FACTOR:
        return _jsymk.getPowerFactor();

    case Field_t::FREQUENCY:
        return _jsymk.getFrequency();

    case Field_t::NEGATIVE:
        return isLogicalNegative() ? 1 : 0;

    case Field_t::TOTAL_POSITIVE_ENERGY:
        return (config.PowerMeter.channel[channel].InvertDirection ? _jsymk.getNegativeEnergy() : _jsymk.getPositiveEnergy());

    case Field_t::TOTAL_NEGATIVE_ENERGY:
        return (config.PowerMeter.channel[channel].InvertDirection ? _jsymk.getPositiveEnergy() : _jsymk.getNegativeEnergy());

    case Field_t::TODAY_POSITIVE_ENERGY:
        return (config.PowerMeter.channel[channel].InvertDirection ? _jsymk.getNegativeEnergy() - _todayNegativeRef : _jsymk.getPositiveEnergy() - _todayPositiveRef);

    case Field_t::TODAY_NEGATIVE_ENERGY:
        return (config.PowerMeter.channel[channel].InvertDirection ? _jsymk.getPositiveEnergy() - _todayPositiveRef : _jsymk.getNegativeEnergy() - _todayNegativeRef);

    default:
        break;
    }

    return 0;
}

String JsyMkClass::getFieldUnit(Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    const auto& fielUnit = std::get<1>(fieldInfos[static_cast<size_t>(fieldId)]);
    return String(fielUnit.data(), fielUnit.size());
}

size_t JsyMkClass::getFieldDigits(Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    return std::get<2>(fieldInfos[static_cast<size_t>(fieldId)]);
}

String JsyMkClass::getFieldDeviceClass(Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    const auto& fieldDeviceClass = std::get<3>(fieldInfos[static_cast<size_t>(fieldId)]);
    return String(fieldDeviceClass.data(), fieldDeviceClass.size());
}

String JsyMkClass::getFieldStatusClass(Field_t fieldId) const
{
    if (static_cast<size_t>(fieldId) >= fieldInfos.size())
        return {};

    const auto& fieldStatusClass = std::get<4>(fieldInfos[static_cast<size_t>(fieldId)]);
    return String(fieldStatusClass.data(), fieldStatusClass.size());
}

void JsyMkClass::reset()
{
    _jsymk.resetEnergy();
}

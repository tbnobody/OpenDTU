// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <MqttSettings.h>
#include <battery/Stats.h>
#include <map>
#include <optional>
#include <Configuration.h>
#include <frozen/string.h>
#include <frozen/map.h>

namespace Batteries::Zendure {

static constexpr const char* invalid = "invalid";

enum class State : uint8_t {
    Idle        = 0,
    Charging    = 1,
    Discharging = 2,
    Invalid     = 255
};

enum class ChargeThroughState : uint8_t {
    Disabled = 0,   // ChargeThrough is disabled by configuration
    Idle = 1,       // ChargeThrough is in standby
    Soft = 2,       // ChargeThrough trying to charge battery to 100 % during normal operation
    Hard = 3,       // ChargeThrough enforcing battery charging to 100 % while setting OutputLimit to 0 W
    Keep = 4        // ChargeThrough keeping target at 100% till discharing or new day begins
};

static constexpr frozen::map<ChargeThroughState, const char*, 5> _chargeThroughStateStrings {
    { ChargeThroughState::Disabled, "disabled" },
    { ChargeThroughState::Idle,     "idle" },
    { ChargeThroughState::Soft,     "soft" },
    { ChargeThroughState::Hard,     "hard" },
    { ChargeThroughState::Keep,     "keep" }
};

static constexpr frozen::map<BatteryZendureConfig::BypassMode_t, const char*, 3> _bypassModeStrings {
    { BatteryZendureConfig::BypassMode_t::Automatic, "automatic" },
    { BatteryZendureConfig::BypassMode_t::AlwaysOff, "alwaysoff" },
    { BatteryZendureConfig::BypassMode_t::AlwaysOn,  "alwayson" }
};


class PackStats;

class Stats : public ::Batteries::Stats {
    friend class Provider;

    static const char* chargeThroughStateToString(std::optional<ChargeThroughState> mode) {
        if (!mode.has_value()) {
            return invalid;
        }

        return chargeThroughStateToString(*mode);
    }

    static const char* chargeThroughStateToString(ChargeThroughState mode) {
        if (_chargeThroughStateStrings.contains(mode)) {
            return _chargeThroughStateStrings.at(mode);
        }

        return invalid;
    }

    static std::optional<ChargeThroughState> chargeThroughStateFromString(String value) {
        for (auto entry : _chargeThroughStateStrings) {
            if (String(entry.second) == value) {
                return entry.first;
            }
        }

        return std::nullopt;
    }

    static const char* controlModeToString(uint8_t controlMode) {
        switch (controlMode) {
            case BatteryZendureConfig::ControlMode::ControlModeFull:
                return "full-access";
            case BatteryZendureConfig::ControlMode::ControlModeOnce:
                return "write-once";
            case BatteryZendureConfig::ControlMode::ControlModeReadOnly:
                return "read-only";
            default:
                break;
        }
        return invalid;
    }

    static const char* stateToString(State state) {
        switch (state) {
            case State::Idle:
                return "idle";
            case State::Charging:
                return "charging";
            case State::Discharging:
                return "discharging";
            default:
                break;
        }
        return invalid;
    }

    static const char* bypassModeToString(BatteryZendureConfig::BypassMode_t mode) {
        if (_bypassModeStrings.contains(mode)) {
            return _bypassModeStrings.at(mode);
        }

        return invalid;
    }

    inline static bool isDischarging(State state) {
        return state == State::Discharging;
    }

    inline static bool isCharging(State state) {
        return state == State::Charging;
    }

public:
    void getLiveViewData(JsonVariant& root) const final;
    void mqttPublish() const final;

    std::optional<String> getHassDeviceName() const final {
        return String(*getManufacturer() + " " + _device);
    }

    bool supportsAlarmsAndWarnings() const final { return false; }

    std::map<size_t, std::shared_ptr<PackStats>> getPackDataList() const { return _packData; }

protected:
    std::shared_ptr<PackStats> getPackData(size_t index) const;
    std::shared_ptr<PackStats> addPackData(size_t index, String serial);

    uint16_t getUseableCapacity() const { return _capacity_avail * (static_cast<float>(_soc_max - _soc_min) / 100.0); };

private:
    void setLastUpdate(uint32_t ts) { _lastUpdate = ts; }

    template<typename T>
    inline static void publish(const String &topic, const T &payload, [[maybe_unused]] const size_t precision = 0) {
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            MqttSettings.publish(topic, String(payload, precision));
            return;
        }

        MqttSettings.publish(topic, String(payload));
    }

    template<typename T>
    inline static void publish(const String &topic, const std::optional<T> &payload, const size_t precision = 0) {
        if (!payload.has_value()) {
            return;
        }

        publish(topic, *payload, precision);
    }

    void setHwVersion(String&& version) {
        if (!version.isEmpty()) {
            _hwversion = _device + " (" + std::move(version) + ")";
        }else{
            _hwversion = _device;
        }
    }
    void setFwVersion(String&& version) { _fwversion = std::move(version); }

    void setSerial(String serial) {
        _serial = serial;
    }
    void setSerial(std::optional<String> serial) {
        if (serial.has_value()) {
            setSerial(*serial);
        }
    }

    void setDevice(String&& device) {
        _device = std::move(device);
    }

    inline void updateSolarInputPower() {
        _input_power = _solar_power_1 + _solar_power_2;
    }

    inline void setSolarPower1(const uint16_t power) {
        _solar_power_1 = power;
        updateSolarInputPower();
    }

    inline void setSolarPower2(const uint16_t power) {
        _solar_power_2 = power;
        updateSolarInputPower();
    }

    void setChargePower(const uint16_t power) {
        _charge_power = power;

        if (power > 0 && _capacity_avail > 0) {
            _remain_in_time = static_cast<uint16_t>(_capacity_avail * (_soc_max - getSoC()) / 100 / static_cast<float>(power) * 60);
        } else {
            _remain_in_time = std::nullopt;
        }
    }

    void setDischargePower(const uint16_t power){
        _discharge_power = power;

        if (power > 0 && _capacity_avail > 0) {
            _remain_out_time = static_cast<uint16_t>(_capacity_avail * (getSoC() - _soc_min) / 100 / static_cast<float>(power) * 60);
        } else {
            _remain_out_time = std::nullopt;
        }
    }

    inline void setOutputPower(const uint16_t power) {
        _output_power = power;
    }

    inline void setOutputLimit(const uint16_t power) {
        _output_limit = power;
    }

    inline void setSocMin(const float soc) {
        // Limit value to 0...60% as Zendure seems to do so, too
        if (soc < 0 || soc > 60) {
            return;
        }
        _soc_min = soc;
    }

    inline void setSocMax(const float soc) {
        // Limit value to 40...100% as Zendure seems to do so, too
        if (soc < 40 || soc > 100) {
            return;
        }
        _soc_max = soc;
    }

    inline void setAutoRecover(const uint8_t value) {
        _auto_recover = static_cast<bool>(value);
    }

    inline void setVoltage(float voltage, uint32_t timestamp) {
        if (voltage > 0) {
            setDischargeCurrentLimit(static_cast<float>(_inverse_max) / voltage, timestamp);
        }
        Batteries::Stats::setVoltage(voltage, timestamp);
    }

    String _device = String("Unknown");

    std::map<size_t, std::shared_ptr<PackStats>> _packData = std::map<size_t, std::shared_ptr<PackStats> >();

    int16_t _cellTemperature = 0;
    uint16_t _cellMinMilliVolt = 0;
    uint16_t _cellMaxMilliVolt = 0;
    uint16_t _cellDeltaMilliVolt = 0;
    uint16_t _cellAvgMilliVolt = 0;

    float _soc_max = 0.0;
    float _soc_min = 0.0;

    uint16_t _inverse_max = 0;
    uint16_t _input_limit = 0;
    uint16_t _output_limit = 0;

    std::optional<float> _efficiency = std::nullopt;
    uint16_t _capacity = 0;
    uint16_t _capacity_avail = 0;

    uint16_t _charge_power = 0;
    uint16_t _discharge_power = 0;
    uint16_t _output_power = 0;
    uint16_t _input_power = 0;
    uint16_t _solar_power_1 = 0;
    uint16_t _solar_power_2 = 0;

    uint16_t _charge_power_cycle = 0;
    uint16_t _discharge_power_cycle = 0;
    uint16_t _output_power_cycle = 0;
    uint16_t _input_power_cycle = 0;
    uint16_t _solar_power_1_cycle = 0;
    uint16_t _solar_power_2_cycle = 0;

    std::optional<uint16_t> _remain_out_time = std::nullopt;
    std::optional<uint16_t> _remain_in_time = std::nullopt;

    State _state = State::Invalid;
    uint8_t _num_batteries = 0;
    BatteryZendureConfig::BypassMode_t _bypass_mode = BatteryZendureConfig::BypassMode_t::Automatic;
    bool _bypass_state = false;
    bool _auto_recover = false;
    bool _heat_state = false;
    bool _auto_shutdown = false;
    bool _buzzer = false;

    std::optional<uint64_t> _last_full_timestamp = std::nullopt;
    std::optional<uint32_t> _last_full_hours = std::nullopt;
    std::optional<uint64_t> _last_empty_timestamp = std::nullopt;
    std::optional<uint32_t> _last_empty_hours = std::nullopt;
    std::optional<ChargeThroughState>  _charge_through_state = std::nullopt;
};

class PackStats {
    friend class Stats;
    friend class Provider;

    public:
        PackStats() {}
        explicit PackStats(String serial) : _serial(serial) {}
        virtual ~PackStats() {}

        String getSerial() const { return _serial; }

        inline uint8_t getCellCount() const { return _cellCount; }
        inline String getName() const { return _name; }

        static std::shared_ptr<PackStats> fromSerial(String serial) {
            if (serial.length() == 15) {
                if (serial.startsWith("AO4H")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AB1000", 960));
                }
                if (serial.startsWith("BO4N")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AB1000S", 960));
                }
                if (serial.startsWith("CO4H")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AB2000", 1920));
                }
                if (serial.startsWith("CO4F")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AB2000S", 1920));
                }
                if (serial.startsWith("CO4E")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AB2000X", 1920));
                }
                if (serial.startsWith("ABB3")) {
                    return std::make_shared<PackStats>(PackStats(serial, "AIO2400", 2400));
                }
                return std::make_shared<PackStats>(PackStats(serial));
            }
            return nullptr;
        };

    protected:
        explicit PackStats(String serial, String name, uint16_t capacity, uint8_t cellCount = 15) :
            _serial(serial), _name(name), _capacity(capacity), _cellCount(cellCount), _capacity_avail(capacity) {}
        void setSerial(String serial) { _serial = serial; }
        void setHwVersion(String&& version) { _hwversion = std::move(version); }
        void setFwVersion(String&& version) { _fwversion = std::move(version); }

        void setSoH(float soh){
            if (soh < 0) {
                return;
            }
            _state_of_health = soh;
            _capacity_avail = _capacity * soh / 100.0;
        }

    private:
        String _serial = String();
        String _name = String("UNKNOWN");
        uint16_t _capacity = 0;
        uint8_t _cellCount = 15;
        uint16_t _capacity_avail = 0;

        String _fwversion = String();
        String _hwversion = String();

        uint16_t _cell_voltage_min = 0;
        uint16_t _cell_voltage_max = 0;
        uint16_t _cell_voltage_spread = 0;
        uint16_t _cell_voltage_avg = 0;
        int16_t _cell_temperature_max = 0;

        std::optional<float> _state_of_health = std::nullopt;

        float _voltage_total = 0.0;
        float _current = 0.0;
        int16_t _power = 0;
        float _soc_level = 0.0;
        State _state = State::Invalid;

        uint32_t _lastUpdate = 0;
};


} // namespace Batteries::Zendure

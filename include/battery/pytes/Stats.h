// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <battery/Stats.h>

namespace Batteries::Pytes {

class Stats : public ::Batteries::Stats {
friend class Provider;

public:
    void getLiveViewData(JsonVariant& root) const final;
    void mqttPublish() const final;
    bool getImmediateChargingRequest() const { return _chargeImmediately; };

private:
    void setLastUpdate(uint32_t ts) { _lastUpdate = ts; }
    void updateSerial() {
        if (!_serialPart1.isEmpty() && !_serialPart2.isEmpty()) {
            _serial = _serialPart1 + _serialPart2;
        }
    }

    String _serialPart1 = "";
    String _serialPart2 = "";

    float _chargeVoltageLimit;
    float _dischargeVoltageLimit;

    uint16_t _stateOfHealth;
    int _chargeCycles = -1;
    int _balance = -1;

    float _temperature;

    uint16_t _cellMinMilliVolt;
    uint16_t _cellMaxMilliVolt;
    float _cellMinTemperature;
    float _cellMaxTemperature;

    String _cellMinVoltageName;
    String _cellMaxVoltageName;
    String _cellMinTemperatureName;
    String _cellMaxTemperatureName;

    uint8_t _moduleCountOnline;
    uint8_t _moduleCountOffline;

    uint8_t _moduleCountBlockingCharge;
    uint8_t _moduleCountBlockingDischarge;

    float _totalCapacity;
    float _availableCapacity;
    uint8_t _capacityPrecision = 0; // decimal places

    float _chargedEnergy = -1;
    float _dischargedEnergy = -1;

    bool _alarmUnderVoltage;
    bool _alarmOverVoltage;
    bool _alarmOverCurrentCharge;
    bool _alarmOverCurrentDischarge;
    bool _alarmUnderTemperature;
    bool _alarmOverTemperature;
    bool _alarmUnderTemperatureCharge;
    bool _alarmOverTemperatureCharge;
    bool _alarmInternalFailure;
    bool _alarmCellImbalance;

    bool _warningLowVoltage;
    bool _warningHighVoltage;
    bool _warningHighChargeCurrent;
    bool _warningHighDischargeCurrent;
    bool _warningLowTemperature;
    bool _warningHighTemperature;
    bool _warningLowTemperatureCharge;
    bool _warningHighTemperatureCharge;
    bool _warningInternalFailure;
    bool _warningCellImbalance;

    bool _chargeImmediately;
};

} // namespace Batteries::Pytes

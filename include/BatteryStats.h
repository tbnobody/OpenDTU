// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>

#include "AsyncJson.h"
#include "Arduino.h"
#include "JkBmsDataPoints.h"
#include "VeDirectShuntController.h"
#include <cfloat>

// mandatory interface for all kinds of batteries
class BatteryStats {
    public:
        String const& getManufacturer() const { return _manufacturer; }

        // the last time *any* datum was updated
        uint32_t getAgeSeconds() const { return (millis() - _lastUpdate) / 1000; }
        bool updateAvailable(uint32_t since) const;

        uint8_t getSoC() const { return _soc; }
        uint32_t getSoCAgeSeconds() const { return (millis() - _lastUpdateSoC) / 1000; }

        float getVoltage() const { return _voltage; }
        uint32_t getVoltageAgeSeconds() const { return (millis() - _lastUpdateVoltage) / 1000; }

        // convert stats to JSON for web application live view
        virtual void getLiveViewData(JsonVariant& root) const;

        void mqttLoop();

        // the interval at which all battery datums will be re-published, even
        // if they did not change. used to calculate Home Assistent expiration.
        virtual uint32_t getMqttFullPublishIntervalMs() const;

        bool isSoCValid() const { return _lastUpdateSoC > 0; }
        bool isVoltageValid() const { return _lastUpdateVoltage > 0; }

        // returns true if the battery reached a critically low voltage/SoC,
        // such that it is in need of charging to prevent degredation.
        virtual bool getImmediateChargingRequest() const { return false; };

        virtual float getChargeCurrent() const { return 0; };
        virtual float getChargeCurrentLimitation() const { return FLT_MAX; };

    protected:
        virtual void mqttPublish() const;

        void setSoC(float soc, uint8_t precision, uint32_t timestamp) {
            _soc = soc;
            _socPrecision = precision;
            _lastUpdateSoC = _lastUpdate = timestamp;
        }

        void setVoltage(float voltage, uint32_t timestamp) {
            _voltage = voltage;
            _lastUpdateVoltage = _lastUpdate = timestamp;
        }

        String _manufacturer = "unknown";
        String _hwversion = "";
        String _fwversion = "";
        String _serial = "";
        uint32_t _lastUpdate = 0;

    private:
        uint32_t _lastMqttPublish = 0;
        float _soc = 0;
        uint8_t _socPrecision = 0; // decimal places
        uint32_t _lastUpdateSoC = 0;
        float _voltage = 0; // total battery pack voltage
        uint32_t _lastUpdateVoltage = 0;
};

class PylontechBatteryStats : public BatteryStats {
    friend class PylontechCanReceiver;

    public:
        void getLiveViewData(JsonVariant& root) const final;
        void mqttPublish() const final;
        bool getImmediateChargingRequest() const { return _chargeImmediately; } ;
        float getChargeCurrent() const { return _current; } ;
        float getChargeCurrentLimitation() const { return _chargeCurrentLimitation; } ;

    private:
        void setManufacturer(String&& m) { _manufacturer = std::move(m); }
        void setLastUpdate(uint32_t ts) { _lastUpdate = ts; }

        float _chargeVoltage;
        float _chargeCurrentLimitation;
        float _dischargeCurrentLimitation;
        uint16_t _stateOfHealth;
        // total current into (positive) or from (negative)
        // the battery, i.e., the charging current
        float _current;
        float _temperature;

        bool _alarmOverCurrentDischarge;
        bool _alarmOverCurrentCharge;
        bool _alarmUnderTemperature;
        bool _alarmOverTemperature;
        bool _alarmUnderVoltage;
        bool _alarmOverVoltage;
        bool _alarmBmsInternal;

        bool _warningHighCurrentDischarge;
        bool _warningHighCurrentCharge;
        bool _warningLowTemperature;
        bool _warningHighTemperature;
        bool _warningLowVoltage;
        bool _warningHighVoltage;
        bool _warningBmsInternal;

        bool _chargeEnabled;
        bool _dischargeEnabled;
        bool _chargeImmediately;
};

class PytesBatteryStats : public BatteryStats {
    friend class PytesCanReceiver;

    public:
        void getLiveViewData(JsonVariant& root) const final;
        void mqttPublish() const final;
        float getChargeCurrent() const { return _current; } ;
        float getChargeCurrentLimitation() const { return _chargeCurrentLimit; } ;

    private:
        void setManufacturer(String&& m) { _manufacturer = std::move(m); }
        void setLastUpdate(uint32_t ts) { _lastUpdate = ts; }
        void updateSerial() {
            if (!_serialPart1.isEmpty() && !_serialPart2.isEmpty()) {
                _serial = _serialPart1 + _serialPart2;
            }
        }

        String _serialPart1 = "";
        String _serialPart2 = "";

        float _chargeVoltageLimit;
        float _chargeCurrentLimit;
        float _dischargeVoltageLimit;
        float _dischargeCurrentLimit;

        uint16_t _stateOfHealth;

        // total current into (positive) or from (negative)
        // the battery, i.e., the charging current
        float _current;
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

        uint16_t _totalCapacity;
        uint16_t _availableCapacity;

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
};

class JkBmsBatteryStats : public BatteryStats {
    public:
        void getLiveViewData(JsonVariant& root) const final {
            getJsonData(root, false);
        }

        void getInfoViewData(JsonVariant& root) const {
            getJsonData(root, true);
        }

        void mqttPublish() const final;

        uint32_t getMqttFullPublishIntervalMs() const final { return 60 * 1000; }

        void updateFrom(JkBms::DataPointContainer const& dp);

    private:
        void getJsonData(JsonVariant& root, bool verbose) const;

        JkBms::DataPointContainer _dataPoints;
        mutable uint32_t _lastMqttPublish = 0;
        mutable uint32_t _lastFullMqttPublish = 0;

        uint16_t _cellMinMilliVolt = 0;
        uint16_t _cellAvgMilliVolt = 0;
        uint16_t _cellMaxMilliVolt = 0;
        uint32_t _cellVoltageTimestamp = 0;
};

class VictronSmartShuntStats : public BatteryStats {
    public:
        void getLiveViewData(JsonVariant& root) const final;
        void mqttPublish() const final;

        void updateFrom(VeDirectShuntController::data_t const& shuntData);

    private:
        float _current;
        float _temperature;
        bool _tempPresent;
        uint8_t _chargeCycles;
        uint32_t _timeToGo;
        float _chargedEnergy;
        float _dischargedEnergy;
        int32_t _instantaneousPower;
        float _midpointVoltage;
        float _midpointDeviation;
        float _consumedAmpHours;
        int32_t _lastFullCharge;

        bool _alarmLowVoltage;
        bool _alarmHighVoltage;
        bool _alarmLowSOC;
        bool _alarmLowTemperature;
        bool _alarmHighTemperature;
};

class MqttBatteryStats : public BatteryStats {
    friend class MqttBattery;

    public:
        // since the source of information was MQTT in the first place,
        // we do NOT publish the same data under a different topic.
        void mqttPublish() const final { }

        // if the voltage is subscribed to at all, it alone does not warrant a
        // card in the live view, since the SoC is already displayed at the top
        void getLiveViewData(JsonVariant& root) const final { }
};

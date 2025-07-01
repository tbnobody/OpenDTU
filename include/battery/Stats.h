// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <AsyncJson.h>
#include <cfloat>
#include <string>

namespace Batteries {

// mandatory interface for all kinds of batteries
class Stats {
public:
    std::optional<String> const& getManufacturer() const { return _oManufacturer; }
    virtual std::optional<String> getHassDeviceName() const { return _oManufacturer; }

    // the last time *any* data was updated
    uint32_t getAgeSeconds() const { return (millis() - _lastUpdate) / 1000; }
    bool updateAvailable(uint32_t since) const;

    float getSoC() const { return _soc; }
    uint32_t getSoCAgeSeconds() const { return (millis() - _lastUpdateSoC) / 1000; }
    uint8_t getSoCPrecision() const { return _socPrecision; }

    float getVoltage() const { return _voltage; }
    uint32_t getVoltageAgeSeconds() const { return (millis() - _lastUpdateVoltage) / 1000; }

    float getChargeCurrent() const { return _current; };
    uint8_t getChargeCurrentPrecision() const { return _currentPrecision; }

    float getDischargeCurrentLimit() const { return _dischargeCurrentLimit; };
    uint32_t getDischargeCurrentLimitAgeSeconds() const { return (millis() - _lastUpdateDischargeCurrentLimit) / 1000; }

    float getChargeCurrentLimit() const { return _chargeCurrentLimit; };
    uint32_t getChargeCurrentLimitAgeSeconds() const { return (millis() - _lastUpdateChargeCurrentLimit) / 1000; }

    // convert stats to JSON for web application live view
    virtual void getLiveViewData(JsonVariant& root) const;

    void mqttLoop();

    // the interval at which all battery data will be re-published, even
    // if they did not change. used to calculate Home Assistent expiration.
    virtual uint32_t getMqttFullPublishIntervalMs() const;

    bool isSoCValid() const { return _lastUpdateSoC > 0; }
    bool isVoltageValid() const { return _lastUpdateVoltage > 0; }
    bool isCurrentValid() const { return _lastUpdateCurrent > 0; }
    bool isDischargeCurrentLimitValid() const { return _lastUpdateDischargeCurrentLimit > 0; }
    bool isChargeCurrentLimitValid() const { return _lastUpdateChargeCurrentLimit > 0; }

    // returns true if the battery reached a critically low voltage/SoC,
    // such that it is in need of charging to prevent degredation.
    virtual bool getImmediateChargingRequest() const { return false; };

    virtual bool supportsAlarmsAndWarnings() const { return true; };

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

    void setCurrent(float current, uint8_t precision, uint32_t timestamp) {
        _current = current;
        _currentPrecision = precision;
        _lastUpdateCurrent = _lastUpdate = timestamp;
    }

    void setDischargeCurrentLimit(float dischargeCurrentLimit, uint32_t timestamp) {
        _dischargeCurrentLimit = dischargeCurrentLimit;
        _lastUpdateDischargeCurrentLimit = _lastUpdate = timestamp;
    }

    void setChargeCurrentLimit(float chargeCurrentLimit, uint32_t timestamp) {
        _chargeCurrentLimit = chargeCurrentLimit;
        _lastUpdateChargeCurrentLimit = _lastUpdate = timestamp;
    }

    void setManufacturer(const String& m);

    template<typename T>
    static void addLiveViewInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        T&& value, std::string const& unit, uint8_t precision)
    {
        auto jsonValue = root["values"][section][name];
        jsonValue["v"] = value;
        jsonValue["u"] = unit;
        jsonValue["d"] = precision;
    }

    template<typename T>
    static void addLiveViewInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        const std::optional<T>& value, std::string const& unit, uint8_t precision, bool hideMissing = false)
    {
        if (value.has_value()) {
            addLiveViewInSection(root, section, name, *value, unit, precision);
        }else if (!hideMissing) {
            addLiveViewTextInSection(root, section, name, "unavail", true);
        }
    }

    static void addLiveViewBooleanInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        bool value, bool translate = true, bool dummy = true)
    {
        addLiveViewTextInSection(root, section, name, value ? "enabled" : "disabled");
    }

    static void addLiveViewBooleanInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        std::optional<bool> value, bool translate = true, bool hideMissing = true)
    {
        if (value.has_value()) {
            addLiveViewBooleanInSection(root, section, name, *value, translate);
        }else if (!hideMissing) {
            addLiveViewTextInSection(root, section, name, "unavail", true);
        }
    }

    template<typename T>
    static void addLiveViewValue(JsonVariant& root, std::string const& name,
        T&& value, std::string const& unit, uint8_t precision)
    {
        addLiveViewInSection(root, "status", name, value, unit, precision);
    }

    static void addLiveViewTextInSection(JsonVariant& root,
        std::string const& section, std::string const& name,
        std::string const& text, bool translate = true)
    {
        auto jsonValue = root["values"][section][name];
        jsonValue["value"] = text;
        jsonValue["translate"] = translate;
    }

    static void addLiveViewTextValue(JsonVariant& root, std::string const& name,
        std::string const& text)
    {
        addLiveViewTextInSection(root, "status", name, text);
    }

    static void addLiveViewWarning(JsonVariant& root, std::string const& name,
        bool warning)
    {
        if (!warning) { return; }
        root["issues"][name] = 1;
    }

    static void addLiveViewAlarm(JsonVariant& root, std::string const& name,
        bool alarm)
    {
        if (!alarm) { return; }
        root["issues"][name] = 2;
    }

    String _hwversion = "";
    String _fwversion = "";
    String _serial = "";
    uint32_t _lastUpdate = 0;

private:
    std::optional<String> _oManufacturer = std::nullopt;
    uint32_t _lastMqttPublish = 0;
    float _soc = 0;
    uint8_t _socPrecision = 0; // decimal places
    uint32_t _lastUpdateSoC = 0;
    float _voltage = 0; // total battery pack voltage
    uint32_t _lastUpdateVoltage = 0;

    // total current into (positive) or from (negative)
    // the battery, i.e., the charging current
    float _current = 0;
    uint8_t _currentPrecision = 0; // decimal places
    uint32_t _lastUpdateCurrent = 0;

    float _dischargeCurrentLimit = 0;
    uint32_t _lastUpdateDischargeCurrentLimit = 0;
    float _chargeCurrentLimit = FLT_MAX;
    uint32_t _lastUpdateChargeCurrentLimit = 0;
};

} // namespace Batteries

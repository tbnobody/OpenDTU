// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <AsyncJson.h>

namespace SolarChargers {

class Stats {
public:
    // the last time *any* data was updated
    virtual uint32_t getAgeMillis() const;

    // Total output of all MPPT charge controllers in Watts.
    // This value can be negative if a charge controller with a load output is used
    // and the load is consuming more power than the charge controller is producing.
    virtual std::optional<float> getOutputPowerWatts() const;

    // minimum of all MPPT charge controllers' output voltages in V
    virtual std::optional<float> getOutputVoltage() const;

    // total panel input power of all MPPT charge controllers in Watts
    virtual std::optional<uint16_t> getPanelPowerWatts() const;

    // sum of total yield of all MPPT charge controllers in kWh
    virtual std::optional<float> getYieldTotal() const;

    // sum of today's yield of all MPPT charge controllers in Wh
    virtual std::optional<float> getYieldDay() const;

    // state of operation from the first available controller
    enum class StateOfOperation : uint8_t { Off = 0, Bulk = 1, Absorption = 2, Float = 3, Various = 255 };
    virtual std::optional<Stats::StateOfOperation> getStateOfOperation() const;

    // float voltage from the first available charge controller in V
    virtual std::optional<float> getFloatVoltage() const;

    // absorption voltage from the first available charge controller in V
    virtual std::optional<float> getAbsorptionVoltage() const;

    // convert stats to JSON for web application live view
    virtual void getLiveViewData(JsonVariant& root, const boolean fullUpdate, const uint32_t lastPublish) const;

    void mqttLoop();

    virtual void mqttPublishSensors(const boolean forcePublish) const;

    // the interval at which all data will be re-published, even
    // if they did not change. used to calculate Home Assistent expiration.
    uint32_t getMqttFullPublishIntervalMs() const;

protected:
    virtual void mqttPublish() const;

private:
    uint32_t _lastMqttPublish = 0;
};

} // namespace SolarChargers

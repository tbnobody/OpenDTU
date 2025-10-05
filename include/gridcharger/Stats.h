// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <AsyncJson.h>

namespace GridChargers {

class Stats {
public:
    virtual uint32_t getLastUpdate() const = 0;

    virtual std::optional<float> getInputPower() const = 0;

    // convert stats to JSON for web application live view
    virtual void getLiveViewData(JsonVariant& root) const = 0;

    void mqttLoop();

    // the interval at which all data will be re-published, even
    // if they did not change. used to calculate Home Assistant expiration.
    static uint32_t getMqttFullPublishIntervalMs();

protected:
    virtual void mqttPublish() const = 0;

    template<typename T>
    static void addValueInSection(JsonVariant& root,
                                  std::string const& section, std::string const& name,
                                  T value, std::string const& unit,
                                  int precision = 2)
    {
        auto jsonValue = root["values"][section][name];
        jsonValue["v"] = value;
        jsonValue["u"] = unit;
        jsonValue["d"] = precision;
    }

    template<typename T>
    static void addStringInSection(JsonVariant& root,
                                   std::string const& section, std::string const& name,
                                   T value, bool translate = true)
    {
        auto jsonValue = root["values"][section][name];
        jsonValue["value"] = value;
        jsonValue["translate"] = translate;
    }

private:
    uint32_t _lastMqttPublish = 0;
};

} // namespace GridChargers

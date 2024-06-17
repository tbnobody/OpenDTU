// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <stdint.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include "Configuration.h"
#include "PowerMeterProvider.h"
#include "sml.h"

class PowerMeterSml : public PowerMeterProvider {
public:
    float getPowerTotal() const final;
    void doMqttPublish() const final;

protected:
    explicit PowerMeterSml(char const* user)
        : _user(user) { }

    void processSmlByte(uint8_t byte);

private:
    std::string _user;
    mutable std::mutex _mutex;

    using values_t = struct {
        std::optional<float> activePowerTotal = std::nullopt;
        std::optional<float> activePowerL1 = std::nullopt;
        std::optional<float> activePowerL2 = std::nullopt;
        std::optional<float> activePowerL3 = std::nullopt;
        std::optional<float> voltageL1 = std::nullopt;
        std::optional<float> voltageL2 = std::nullopt;
        std::optional<float> voltageL3 = std::nullopt;
        std::optional<float> currentL1 = std::nullopt;
        std::optional<float> currentL2 = std::nullopt;
        std::optional<float> currentL3 = std::nullopt;
        std::optional<float> energyImport = std::nullopt;
        std::optional<float> energyExport = std::nullopt;
    };

    values_t _values;
    values_t _cache;

    using OBISHandler = struct {
        uint8_t const OBIS[6];
        void (*decoder)(float&);
        std::optional<float>* target;
        char const* name;
    };

    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_cache.activePowerTotal, "active power total"},
        {{0x01, 0x00, 0x24, 0x07, 0x00, 0xff}, &smlOBISW, &_cache.activePowerL1, "active power L1"},
        {{0x01, 0x00, 0x38, 0x07, 0x00, 0xff}, &smlOBISW, &_cache.activePowerL2, "active power L2"},
        {{0x01, 0x00, 0x4c, 0x07, 0x00, 0xff}, &smlOBISW, &_cache.activePowerL3, "active power L3"},
        {{0x01, 0x00, 0x20, 0x07, 0x00, 0xff}, &smlOBISVolt, &_cache.voltageL1, "voltage L1"},
        {{0x01, 0x00, 0x34, 0x07, 0x00, 0xff}, &smlOBISVolt, &_cache.voltageL2, "voltage L2"},
        {{0x01, 0x00, 0x48, 0x07, 0x00, 0xff}, &smlOBISVolt, &_cache.voltageL3, "voltage L3"},
        {{0x01, 0x00, 0x1f, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_cache.currentL1, "current L1"},
        {{0x01, 0x00, 0x33, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_cache.currentL2, "current L2"},
        {{0x01, 0x00, 0x47, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_cache.currentL3, "current L3"},
        {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &smlOBISWh, &_cache.energyImport, "energy import"},
        {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, &smlOBISWh, &_cache.energyExport, "energy export"}
    };
};

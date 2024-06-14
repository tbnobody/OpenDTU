// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <list>
#include <mutex>
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

    float _activePowerTotal = 0.0;
    float _activePowerL1 = 0.0;
    float _activePowerL2 = 0.0;
    float _activePowerL3 = 0.0;
    float _voltageL1 = 0.0;
    float _voltageL2 = 0.0;
    float _voltageL3 = 0.0;
    float _currentL1 = 0.0;
    float _currentL2 = 0.0;
    float _currentL3 = 0.0;
    float _energyImport = 0.0;
    float _energyExport = 0.0;

    typedef struct {
        uint8_t const OBIS[6];
        void (*decoder)(float&);
        float* target;
        char const* name;
    } OBISHandler;

    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_activePowerTotal, "active power total"},
        {{0x01, 0x00, 0x24, 0x07, 0x00, 0xff}, &smlOBISW, &_activePowerL1, "active power L1"},
        {{0x01, 0x00, 0x38, 0x07, 0x00, 0xff}, &smlOBISW, &_activePowerL2, "active power L2"},
        {{0x01, 0x00, 0x4c, 0x07, 0x00, 0xff}, &smlOBISW, &_activePowerL3, "active power L3"},
        {{0x01, 0x00, 0x20, 0x07, 0x00, 0xff}, &smlOBISVolt, &_voltageL1, "voltage L1"},
        {{0x01, 0x00, 0x34, 0x07, 0x00, 0xff}, &smlOBISVolt, &_voltageL2, "voltage L2"},
        {{0x01, 0x00, 0x48, 0x07, 0x00, 0xff}, &smlOBISVolt, &_voltageL3, "voltage L3"},
        {{0x01, 0x00, 0x1f, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_currentL1, "current L1"},
        {{0x01, 0x00, 0x33, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_currentL2, "current L2"},
        {{0x01, 0x00, 0x47, 0x07, 0x00, 0xff}, &smlOBISAmpere, &_currentL3, "current L3"},
        {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyImport, "energy import"},
        {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyExport, "energy export"}
    };
};

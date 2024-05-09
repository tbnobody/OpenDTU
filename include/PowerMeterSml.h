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
    void processSmlByte(uint8_t byte);

private:
    mutable std::mutex _mutex;

    float _activePower = 0.0;
    float _energyImport = 0.0;
    float _energyExport = 0.0;

    typedef struct {
        uint8_t const OBIS[6];
        void (*decoder)(float&);
        float* target;
        char const* name;
    } OBISHandler;

    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_activePower, "active power"},
        {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyImport, "energy import"},
        {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyExport, "energy export"}
    };
};

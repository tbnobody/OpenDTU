// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterProvider.h"
#include "Configuration.h"
#include "sml.h"
#include <SoftwareSerial.h>
#include <list>
#include <mutex>

class PowerMeterSerialSml : public PowerMeterProvider {
public:
    bool init() final;
    void deinit() final;
    void loop() final;
    float getPowerTotal() const final { return _activePower; }
    void doMqttPublish() const final;

private:
    float _activePower = 0.0;
    float _energyImport = 0.0;
    float _energyExport = 0.0;

    mutable std::mutex _mutex;

    std::unique_ptr<SoftwareSerial> _upSmlSerial = nullptr;

    typedef struct {
        const unsigned char OBIS[6];
        void (*Fn)(double&);
        float* Arg;
    } OBISHandler;

    const std::list<OBISHandler> smlHandlerList{
        {{0x01, 0x00, 0x10, 0x07, 0x00, 0xff}, &smlOBISW, &_activePower},
        {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyImport},
        {{0x01, 0x00, 0x02, 0x08, 0x00, 0xff}, &smlOBISWh, &_energyExport}
    };
};

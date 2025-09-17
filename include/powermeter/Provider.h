// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <Configuration.h>
#include <powermeter/DataPoints.h>

namespace PowerMeters {

class Provider {
public:
    virtual ~Provider() { }

    enum class Type : unsigned {
        MQTT = 0,
        SDM1PH = 1,
        SDM3PH = 2,
        HTTP_JSON = 3,
        SERIAL_SML = 4,
        SMAHM2 = 5,
        HTTP_SML = 6,
        MODBUS_UDP_VICTRON = 7
    };

    // returns true if the provider is ready for use, false otherwise
    virtual bool init() = 0;

    virtual void loop() = 0;
    virtual bool isDataValid() const;

    float getPowerTotal() const;
    uint32_t getLastUpdate() const { return _dataCurrent.getLastUpdate(); }
    void mqttLoop() const;

protected:
    Provider() = default;

    DataPointContainer _dataCurrent;

private:
    mutable uint32_t _lastMqttPublish = 0;
};

} // namespace PowerMeters

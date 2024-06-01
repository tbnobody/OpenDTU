// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <map>

class SerialPortManagerClass {
public:
    void init();
    bool allocateMpptPort(uint8_t port);
    bool allocateBatteryPort(uint8_t port);
    void invalidateBatteryPort();
    void invalidateMpptPorts();

private:
    enum class Owner {
        Console,
        Battery,
        MPPT
    };

    std::map<uint8_t, Owner> allocatedPorts;

    bool allocatePort(uint8_t port, Owner owner);
    void invalidate(Owner owner);

    static const char* print(Owner owner);
};

extern SerialPortManagerClass SerialPortManager;

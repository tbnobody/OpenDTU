// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <map>

class SerialPortManager {
public:
    bool allocateMpptPort(int port);
    bool allocateBatteryPort(int port);
    void invalidateBatteryPort();
    void invalidateMpptPorts();

private:
    enum Owner {
        BATTERY,
        MPPT
    };

    std::map<uint8_t, Owner> allocatedPorts;

    bool allocatePort(uint8_t port, Owner owner);
    void invalidate(Owner owner);

    static const char* print(Owner owner);
};

extern SerialPortManager PortManager;

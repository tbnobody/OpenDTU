// SPDX-License-Identifier: GPL-2.0-or-later
#include "SerialPortManager.h"
#include "MessageOutput.h"

#define MAX_CONTROLLERS 3

SerialPortManagerClass SerialPortManager;

void SerialPortManagerClass::init()
{
    if (ARDUINO_USB_CDC_ON_BOOT != 1) {
        allocatePort(0, Owner::Console);
    }
}

bool SerialPortManagerClass::allocateBatteryPort(uint8_t port)
{
    return allocatePort(port, Owner::Battery);
}

bool SerialPortManagerClass::allocateMpptPort(uint8_t port)
{
    return allocatePort(port, Owner::MPPT);
}

bool SerialPortManagerClass::allocatePort(uint8_t port, Owner owner)
{
    if (port >= MAX_CONTROLLERS) {
        MessageOutput.printf("[SerialPortManager] Invalid serial port: %d\r\n", port);
        return false;
    }

    auto res = allocatedPorts.insert({port, owner});

    if (!res.second) {
        MessageOutput.printf("[SerialPortManager] Cannot assign HW UART "
                "port %d to %s: already in use by %s\r\n",
                port, print(owner), print(res.first->second));
        return false;
    }

    MessageOutput.printf("[SerialPortManager] HW UART port %d now in use "
            "by %s\r\n", port, print(owner));
    return true;
}

void SerialPortManagerClass::invalidateBatteryPort()
{
    invalidate(Owner::Battery);
}

void SerialPortManagerClass::invalidateMpptPorts()
{
    invalidate(Owner::MPPT);
}

void SerialPortManagerClass::invalidate(Owner owner)
{
    for (auto it = allocatedPorts.begin(); it != allocatedPorts.end();) {
        if (it->second == owner) {
            MessageOutput.printf("[SerialPortManager] Removing port = %d, owner = %s \r\n", it->first, print(owner));
            it = allocatedPorts.erase(it);
        } else {
            ++it;
        }
    }
}

const char* SerialPortManagerClass::print(Owner owner)
{
    switch (owner) {
        case Owner::Console:
            return "Serial Console";
        case Owner::Battery:
            return "Battery Interface";
        case Owner::MPPT:
            return "Victron MPPT";
    }
    return "unknown";
}

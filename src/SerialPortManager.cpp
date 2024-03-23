// SPDX-License-Identifier: GPL-2.0-or-later
#include "SerialPortManager.h"
#include "MessageOutput.h"

#define MAX_CONTROLLERS 3

SerialPortManagerClass SerialPortManager;

bool SerialPortManagerClass::allocateBatteryPort(int port)
{
    return allocatePort(port, Owner::BATTERY);
}

bool SerialPortManagerClass::allocateMpptPort(int port)
{
    return allocatePort(port, Owner::MPPT);
}

bool SerialPortManagerClass::allocatePort(uint8_t port, Owner owner)
{
    if (port >= MAX_CONTROLLERS) {
        MessageOutput.printf("[SerialPortManager] Invalid serial port = %d \r\n", port);
        return false;
    }

    return allocatedPorts.insert({port, owner}).second;
}

void SerialPortManagerClass::invalidateBatteryPort()
{
    invalidate(Owner::BATTERY);
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
        case BATTERY:
            return "BATTERY";
        case MPPT:
            return "MPPT";
    }
    return "unknown";
}

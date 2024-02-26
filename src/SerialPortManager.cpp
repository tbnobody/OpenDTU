// SPDX-License-Identifier: GPL-2.0-or-later
#include "SerialPortManager.h"
#include "MessageOutput.h"

#define MAX_CONTROLLERS 3

SerialPortManager PortManager;

bool SerialPortManager::allocateBatteryPort(int port)
{
    return allocatePort(port, Owner::BATTERY);
}

bool SerialPortManager::allocateMpptPort(int port)
{
    return allocatePort(port, Owner::MPPT);
}

bool SerialPortManager::allocatePort(uint8_t port, Owner owner)
{
    if (port >= MAX_CONTROLLERS) {
        MessageOutput.printf("[SerialPortManager] Invalid serial port = %d \r\n", port);
        return false;
    }

    return allocatedPorts.insert({port, owner}).second;
}

void SerialPortManager::invalidateBatteryPort()
{
    invalidate(Owner::BATTERY);
}

void SerialPortManager::invalidateMpptPorts()
{
    invalidate(Owner::MPPT);
}

void SerialPortManager::invalidate(Owner owner)
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

const char* SerialPortManager::print(Owner owner)
{
    switch (owner) {
        case BATTERY:
            return "BATTERY";
        case MPPT:
            return "MPPT";
    }
}

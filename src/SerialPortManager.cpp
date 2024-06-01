// SPDX-License-Identifier: GPL-2.0-or-later
#include "SerialPortManager.h"
#include "MessageOutput.h"

SerialPortManagerClass SerialPortManager;

void SerialPortManagerClass::init()
{
    if (ARDUINO_USB_CDC_ON_BOOT != 1) {
        _ports[0] = "Serial Console";
        MessageOutput.println("[SerialPortManager] HW UART port 0 now in use "
                "by 'Serial Console'");
    }
}

std::optional<uint8_t> SerialPortManagerClass::allocatePort(std::string const& owner)
{
    for (size_t i = 0; i < _ports.size(); ++i) {
        if (_ports[i] != "") {
            MessageOutput.printf("[SerialPortManager] HW UART %d already "
                    "in use by '%s'\r\n", i, _ports[i].c_str());
            continue;
        }

        _ports[i] = owner;

        MessageOutput.printf("[SerialPortManager] HW UART %d now in use "
                "by '%s'\r\n", i, owner.c_str());

        return i;
    }

    MessageOutput.printf("[SerialPortManager] Cannot assign another HW "
            "UART port to '%s'\r\n", owner.c_str());
    return std::nullopt;
}

void SerialPortManagerClass::freePort(std::string const& owner)
{
    for (size_t i = 0; i < _ports.size(); ++i) {
        if (_ports[i] != owner) { continue; }

        MessageOutput.printf("[SerialPortManager] Freeing HW UART %d, owner "
                "was '%s'\r\n", i, owner.c_str());
        _ports[i] = "";
    }
}

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <optional>
#include <string>

class SerialPortManagerClass {
public:
    void init();

    std::optional<uint8_t> allocatePort(std::string const& owner);
    void freePort(std::string const& owner);

private:
    // the amount of hardare UARTs available on supported ESP32 chips
    static size_t constexpr _num_controllers = 3;
    std::array<std::string, _num_controllers> _ports = { "" };
};

extern SerialPortManagerClass SerialPortManager;

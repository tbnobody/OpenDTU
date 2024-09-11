// SPDX-License-Identifier: GPL-2.0-or-later
#include "SPIPortManager.h"
#include "MessageOutput.h"

SPIPortManagerClass SPIPortManager;
static constexpr char  TAG[] = "[SPIPortManager]";

void SPIPortManagerClass::init() {
    MessageOutput.printf("%s SPI0 and SPI1 reserved by 'Flash and PSRAM'\r\n", TAG);
    _ports[0] = "Flash";
    _ports[1] = "PSRAM";
}

std::optional<uint8_t> SPIPortManagerClass::allocatePort(std::string const& owner)
{
    for (size_t i = 0; i < _ports.size(); ++i) {
        if (_ports[i] != "") {
            MessageOutput.printf("%s SPI%d already in use by '%s'\r\n", TAG, i, _ports[i].c_str());
            continue;
        }

        _ports[i] = owner;

        MessageOutput.printf("%s SPI%d now in use by '%s'\r\n", TAG, i, owner.c_str());

        return i + _offset_spi_num;
    }

    MessageOutput.printf("%s Cannot assign another SPI port to '%s'\r\n", TAG, owner.c_str());
    return std::nullopt;
}

void SPIPortManagerClass::freePort(std::string const& owner)
{
    for (size_t i = 0; i < _ports.size(); ++i) {
        if (_ports[i] != owner) { continue; }

        MessageOutput.printf("%s Freeing SPI%d, owner was '%s'\r\n", TAG, i + _offset_spi_num, owner.c_str());
        _ports[i] = "";
    }
}

spi_host_device_t SPIPortManagerClass::SPIhostNum(uint8_t spi_num)
{
    return (spi_host_device_t)(spi_num + _offset_spi_host);
}

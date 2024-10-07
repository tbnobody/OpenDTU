// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <driver/spi_master.h>
#include <memory>

// Pre and post callbacks for 2 buses with 3 devices each
#define SPI_MANAGER_CALLBACK_COUNT 6

class SpiBus;
class SpiBusConfig;

namespace SpiCallback {
bool patch(const std::shared_ptr<SpiBus>& bus, const std::shared_ptr<SpiBusConfig>& bus_config, spi_device_interface_config_t& device_config);
}

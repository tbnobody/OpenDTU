// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiManager.h"

#ifdef ARDUINO
#include <SPI.h>
#endif

SpiManager::SpiManager()
{
}

#ifdef ARDUINO

std::optional<uint8_t> SpiManager::to_arduino(spi_host_device_t host_device)
{
    switch (host_device) {
#if CONFIG_IDF_TARGET_ESP32
    case SPI1_HOST:
        return FSPI;
    case SPI2_HOST:
        return HSPI;
    case SPI3_HOST:
        return VSPI;
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    case SPI2_HOST:
        return FSPI;
    case SPI3_HOST:
        return HSPI;
#elif CONFIG_IDF_TARGET_ESP32C3
    case SPI2_HOST:
        return FSPI;
#endif
    default:
        return std::nullopt;
    }
}

#endif

bool SpiManager::register_bus(spi_host_device_t host_device)
{
    for (int i = 0; i < SPI_MANAGER_NUM_BUSES; ++i) {
        if (available_buses[i])
            continue;

        available_buses[i] = host_device;
        return true;
    }

    return false;
}

bool SpiManager::claim_bus(spi_host_device_t& host_device)
{
    for (int i = SPI_MANAGER_NUM_BUSES - 1; i >= 0; --i) {
        if (!available_buses[i])
            continue;

        host_device = *available_buses[i];
        available_buses[i].reset();
        return true;
    }

    return false;
}

#ifdef ARDUINO

std::optional<uint8_t> SpiManager::claim_bus_arduino()
{
    spi_host_device_t host_device;
    if (!claim_bus(host_device))
        return std::nullopt;
    return to_arduino(host_device);
}

#endif

spi_device_handle_t SpiManager::alloc_device(const std::string& bus_id, const std::shared_ptr<SpiBusConfig>& bus_config, spi_device_interface_config_t& device_config)
{
    std::shared_ptr<SpiBus> shared_bus = get_shared_bus(bus_id);
    if (!shared_bus)
        return nullptr;

    return shared_bus->add_device(bus_config, device_config);
}

std::shared_ptr<SpiBus> SpiManager::get_shared_bus(const std::string& bus_id)
{
    // look for existing shared bus
    for (int i = 0; i < SPI_MANAGER_NUM_BUSES; ++i) {
        if (!shared_buses[i])
            continue;
        if (shared_buses[i]->get_id() == bus_id)
            return shared_buses[i];
    }

    // create new shared bus
    for (int i = 0; i < SPI_MANAGER_NUM_BUSES; ++i) {
        if (shared_buses[i])
            continue;

        spi_host_device_t host_device;
        if (!claim_bus(host_device))
            return nullptr;

        shared_buses[i] = std::make_shared<SpiBus>(bus_id, host_device);
        return shared_buses[i];
    }

    return nullptr;
}

SpiManager SpiManagerInst;

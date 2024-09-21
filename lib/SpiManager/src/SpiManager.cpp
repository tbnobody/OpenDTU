#include "SpiManager.h"

SpiManager::SpiManager() {
}

bool SpiManager::register_bus(spi_host_device_t host_device) {
    for (int i = 0; i < SPI_MANAGER_NUM_BUSES; ++i) {
        if (available_buses[i])
            continue;

        available_buses[i] = host_device;
        return true;
    }

    return false;
}

bool SpiManager::claim_bus(spi_host_device_t &host_device) {
    for (int i = SPI_MANAGER_NUM_BUSES - 1; i >= 0; --i) {
        if (!available_buses[i])
            continue;

        host_device = *available_buses[i];
        available_buses[i].reset();
        return true;
    }

    return false;
}

spi_device_handle_t SpiManager::alloc_device(const std::string &bus_id, const std::shared_ptr<SpiBusConfig> &bus_config, spi_device_interface_config_t &device_config) {
    std::shared_ptr<SpiBus> shared_bus = get_shared_bus(bus_id);
    if (!shared_bus)
        return nullptr;

    return shared_bus->add_device(bus_config, device_config);
}

std::shared_ptr<SpiBus> SpiManager::get_shared_bus(const std::string &bus_id) {
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

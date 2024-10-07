// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <driver/spi_master.h>
#include <memory>
#include <string>

class SpiBusConfig;

class SpiBus : public std::enable_shared_from_this<SpiBus> {
public:
    explicit SpiBus(const std::string& id, spi_host_device_t host_device);
    SpiBus(const SpiBus&) = delete;
    SpiBus& operator=(const SpiBus&) = delete;
    ~SpiBus();

    inline __attribute__((always_inline)) void require_config(SpiBusConfig* config)
    {
        if (config == cur_config)
            return;
        apply_config(config);
    }

    inline __attribute__((always_inline)) void free_config(SpiBusConfig* config)
    {
        if (config != cur_config)
            return;
        apply_config(nullptr);
    }

    inline const std::string& get_id() const
    {
        return id;
    }

    inline spi_host_device_t get_host_device() const
    {
        return host_device;
    }

    spi_device_handle_t add_device(const std::shared_ptr<SpiBusConfig>& bus_config, spi_device_interface_config_t& device_config);

private:
    void apply_config(SpiBusConfig* config);

    std::string id;
    spi_host_device_t host_device;
    SpiBusConfig* cur_config;
};

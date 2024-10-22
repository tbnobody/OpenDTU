// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <hal/gpio_types.h>
#include <hal/spi_types.h>

class SpiBusConfig {
public:
    explicit SpiBusConfig(gpio_num_t pin_mosi, gpio_num_t pin_miso, gpio_num_t pin_sclk);
    SpiBusConfig(const SpiBusConfig&) = delete;
    SpiBusConfig& operator=(const SpiBusConfig&) = delete;
    ~SpiBusConfig();

    void patch(spi_host_device_t host_device);
    void unpatch(spi_host_device_t host_device);

private:
    gpio_num_t pin_mosi;
    gpio_num_t pin_miso;
    gpio_num_t pin_sclk;
};

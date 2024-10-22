// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <driver/spi_master.h>
#include <esp_eth.h> // required for esp_eth_handle_t
#include <esp_netif.h>

#include <memory>

class W5500 {
private:
    explicit W5500(spi_device_handle_t spi, gpio_num_t pin_int);

public:
    W5500(const W5500&) = delete;
    W5500& operator=(const W5500&) = delete;
    ~W5500();

    static std::unique_ptr<W5500> setup(int8_t pin_mosi, int8_t pin_miso, int8_t pin_sclk, int8_t pin_cs, int8_t pin_int, int8_t pin_rst);
    String macAddress();

private:
    static bool connection_check_spi(spi_device_handle_t spi);
    static bool connection_check_interrupt(gpio_num_t pin_int);

    esp_eth_handle_t eth_handle;
    esp_netif_t* eth_netif;
};

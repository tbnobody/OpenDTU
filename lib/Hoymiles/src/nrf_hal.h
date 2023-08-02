#pragma once

#include "spi_patcher.h"

#include <RF24_hal.h>

class nrf_hal : public RF24_hal, public spi_patcher_handle
{
public:
    nrf_hal(gpio_num_t pin_mosi, gpio_num_t pin_miso, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_en);

    void patch() override;
    void unpatch() override;

    bool begin() override;
    void end() override;

    void ce(bool level) override;
    uint8_t write(uint8_t cmd, const uint8_t* buf, uint8_t len) override;
    uint8_t write(uint8_t cmd, const uint8_t* buf, uint8_t data_len, uint8_t blank_len) override;
    uint8_t read(uint8_t cmd, uint8_t* buf, uint8_t len) override;
    uint8_t read(uint8_t cmd, uint8_t* buf, uint8_t data_len, uint8_t blank_len) override;

private:
    inline void request_spi()
    {
        spi_patcher_inst.request(this);
    }

    inline void release_spi()
    {
        spi_patcher_inst.release();
    }

    const gpio_num_t pin_mosi;
    const gpio_num_t pin_miso;
    const gpio_num_t pin_clk;
    const gpio_num_t pin_cs;
    const gpio_num_t pin_en;

    spi_host_device_t host_device;
    spi_device_handle_t spi;
};

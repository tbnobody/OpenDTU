#pragma once

#include "SpiPatcher.h"

#include <RF24_hal.h>

class nrf_hal : public RF24_hal, public SpiPatcherHandle
{
public:
    nrf_hal();

    void patch() override;
    void unpatch() override;

    void init(gpio_num_t pin_mosi, gpio_num_t pin_miso, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_en, int32_t spi_speed = 0);

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
        HoymilesSpiPatcher.request(this);
    }

    inline void release_spi()
    {
        HoymilesSpiPatcher.release();
    }

    gpio_num_t pin_mosi;
    gpio_num_t pin_miso;
    gpio_num_t pin_clk;
    gpio_num_t pin_cs;
    gpio_num_t pin_en;
    int32_t spi_speed;

    spi_host_device_t host_device;
    spi_device_handle_t spi;
};

#pragma once

#include "SpiPatcher.h"

#include <driver/gpio.h>

class cmt_hal : public SpiPatcherHandle
{
public:
    cmt_hal();

    void patch() override;
    void unpatch() override;

    void init(gpio_num_t pin_sdio, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_fcs, int32_t spi_speed = 0);

    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t data);
    void read_fifo(uint8_t* buf, uint16_t len);
    void write_fifo(const uint8_t* buf, uint16_t len);

private:
    inline void request_spi()
    {
        HoymilesSpiPatcher.request(this);
    }

    inline void release_spi()
    {
        HoymilesSpiPatcher.release();
    }

    gpio_num_t pin_sdio;
    gpio_num_t pin_clk;
    gpio_num_t pin_cs;
    gpio_num_t pin_fcs;
    int32_t spi_speed;

    spi_host_device_t host_device;
    spi_device_handle_t spi_reg;
    spi_device_handle_t spi_fifo;
};

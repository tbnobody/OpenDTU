#pragma once

#include "spi_patcher.h"

#include <driver/gpio.h>

class cmt_hal : public spi_patcher_handle
{
public:
    cmt_hal();

    void patch(spi_host_device_t host_device) override;
    void unpatch(spi_host_device_t host_device) override;

    void init(gpio_num_t pin_sdio, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_fcs, int32_t spi_speed);

    uint8_t read_reg(uint8_t addr);
    void write_reg(uint8_t addr, uint8_t data);
    void read_fifo(uint8_t* buf, uint16_t len);
    void write_fifo(const uint8_t* buf, uint16_t len);

private:
    inline void request_spi()
    {
        spi_patcher_inst.request(this);
    }

    inline void release_spi()
    {
        spi_patcher_inst.release();
    }

    gpio_num_t pin_sdio;
    gpio_num_t pin_clk;
    gpio_num_t pin_cs;
    gpio_num_t pin_fcs;
    int32_t spi_speed;

    spi_device_handle_t spi_reg;
    spi_device_handle_t spi_fifo;
};

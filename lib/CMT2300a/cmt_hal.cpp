#include "cmt_hal.h"

#include <Arduino.h>

#define CMT_MAX_TRANSFER_SZ 32

cmt_hal::cmt_hal() :
    pin_sdio(GPIO_NUM_NC),
    pin_clk(GPIO_NUM_NC),
    pin_cs(GPIO_NUM_NC),
    pin_fcs(GPIO_NUM_NC),
    spi_speed(1000000)
{

}

void cmt_hal::patch(spi_host_device_t host_device)
{
    gpio_hold_en(pin_cs);
    gpio_hold_en(pin_fcs);

    gpio_reset_pin(pin_cs);
    gpio_reset_pin(pin_fcs);

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_sdio,
        .miso_io_num = -1, // Single wire MOSI/MISO
        .sclk_io_num = pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = CMT_MAX_TRANSFER_SZ,
        .flags = 0,
        .intr_flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host_device, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg_reg = {
        .command_bits = 1,
        .address_bits = 7,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = spi_speed,
        .input_delay_ns = 0,
        .spics_io_num = pin_cs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg_reg, &spi_reg));

    spi_device_interface_config_t devcfg_fifo = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 2,
        .cs_ena_posttrans = (uint8_t)(1 / (spi_speed * 10e6 * 2) + 2), // >2 us
        .clock_speed_hz = spi_speed,
        .input_delay_ns = 0,
        .spics_io_num = pin_fcs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg_fifo, &spi_fifo));

    gpio_hold_dis(pin_cs);
    gpio_hold_dis(pin_fcs);
}

void cmt_hal::unpatch(spi_host_device_t host_device)
{
    gpio_hold_en(pin_cs);
    gpio_hold_en(pin_fcs);

    spi_bus_remove_device(spi_reg);
    spi_bus_remove_device(spi_fifo);
    spi_bus_free(host_device);

    gpio_reset_pin(pin_cs);
    gpio_set_direction(pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_cs, 1);

    gpio_reset_pin(pin_fcs);
    gpio_set_direction(pin_fcs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_fcs, 1);

    gpio_hold_dis(pin_cs);
    gpio_hold_dis(pin_fcs);
}

void cmt_hal::init(gpio_num_t _pin_sdio, gpio_num_t _pin_clk, gpio_num_t _pin_cs, gpio_num_t _pin_fcs, int32_t _spi_speed)
{
    pin_sdio = _pin_sdio;
    pin_clk = _pin_clk;
    pin_cs = _pin_cs;
    pin_fcs = _pin_fcs;
    spi_speed = _spi_speed;

    gpio_reset_pin(pin_cs);
    gpio_set_direction(pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_cs, 1);

    gpio_reset_pin(pin_fcs);
    gpio_set_direction(pin_fcs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_fcs, 1);
}

uint8_t cmt_hal::read_reg(uint8_t addr)
{
    uint8_t data;

    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 1,
        .addr = addr,
        .length = 0,
        .rxlength = 8,
        .user = NULL,
        .tx_buffer = NULL,
        .rx_buffer = &data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));

    release_spi();

    delayMicroseconds(100);

    return data;
}

void cmt_hal::write_reg(uint8_t addr, uint8_t data)
{
    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = addr,
        .length = 8,
        .rxlength = 0,
        .user = NULL,
        .tx_buffer = &data,
        .rx_buffer = NULL
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));

    release_spi();

    delayMicroseconds(100);
}

void cmt_hal::read_fifo(uint8_t* buf, uint16_t len)
{
    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 0,
        .rxlength = 8,
        .user = NULL,
        .tx_buffer = NULL,
        .rx_buffer = NULL
    };
    for (uint16_t i = 0; i < len; i++) {
        t.rx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
    }

    release_spi();
}

void cmt_hal::write_fifo(const uint8_t* buf, uint16_t len)
{
    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 8,
        .rxlength = 0,
        .user = NULL,
        .tx_buffer = NULL,
        .rx_buffer = NULL
    };
    for (uint16_t i = 0; i < len; i++) {
        t.tx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
    }

    release_spi();
}

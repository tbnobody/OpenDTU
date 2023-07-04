#include "cmt_spi3.h"
#include "cmt_spi_patcher_handle.h"
#include <Arduino.h>

spi_device_handle_t spi_reg, spi_fifo;

int8_t m_pin_sdio, m_pin_clk, m_pin_cs, m_pin_fcs;
uint32_t m_spi_speed;

void cmt_patch_spi(spi_host_device_t host_device)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = m_pin_sdio,
        .miso_io_num = -1, // single wire MOSI/MISO
        .sclk_io_num = m_pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(host_device, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg = {
        .command_bits = 1,
        .address_bits = 7,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = (int)m_spi_speed,
        .spics_io_num = m_pin_cs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg, &spi_reg));

    // FiFo
    spi_device_interface_config_t devcfg2 = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .cs_ena_pretrans = 2,
        .cs_ena_posttrans = (uint8_t)(1 / (m_spi_speed * 10e6 * 2) + 2), // >2 us
        .clock_speed_hz = (int)m_spi_speed,
        .spics_io_num = m_pin_fcs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg2, &spi_fifo));
}

void cmt_unpatch_spi(spi_host_device_t host_device)
{
    spi_bus_remove_device(spi_reg);
    spi_bus_remove_device(spi_fifo);
    spi_bus_free(host_device);
}

void cmt_spi3_init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, uint32_t spi_speed)
{
    m_pin_sdio = pin_sdio;
    m_pin_clk = pin_clk;
    m_pin_cs = pin_cs;
    m_pin_fcs = pin_fcs;
    m_spi_speed = spi_speed;
}

void cmt_spi3_write(uint8_t addr, uint8_t data)
{
    spi_transaction_t t = {
        .cmd = 0,
        .addr = addr,
        .length = 8,
        .tx_buffer = &data,
        .rx_buffer = NULL
    };
    cmt_request_spi();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    cmt_release_spi();
    delayMicroseconds(100);
}

uint8_t cmt_spi3_read(uint8_t addr)
{
    uint8_t rx_data;
    spi_transaction_t t = {
        .cmd = 1,
        .addr = addr,
        .rxlength = 8,
        .tx_buffer = NULL,
        .rx_buffer = &rx_data
    };
    cmt_request_spi();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    cmt_release_spi();
    delayMicroseconds(100);
    return rx_data;
}

void cmt_spi3_write_fifo(const uint8_t* buf, uint16_t len)
{
    spi_transaction_t t = {
        .length = 8,
        .rx_buffer = NULL
    };

    cmt_request_spi();
    for (uint8_t i = 0; i < len; i++) {
        t.tx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
    }
    cmt_release_spi();
}

void cmt_spi3_read_fifo(uint8_t* buf, uint16_t len)
{
    spi_transaction_t t = {
        .rxlength = 8,
        .tx_buffer = NULL
    };

    cmt_request_spi();
    for (uint8_t i = 0; i < len; i++) {
        t.rx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
    }
    cmt_release_spi();
}

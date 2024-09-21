#include "cmt_spi3.h"
#include <Arduino.h>
#include <driver/spi_master.h>
#include <SpiManager.h>

SemaphoreHandle_t paramLock = NULL;
#define SPI_PARAM_LOCK() \
    do {                 \
    } while (xSemaphoreTake(paramLock, portMAX_DELAY) != pdPASS)
#define SPI_PARAM_UNLOCK() xSemaphoreGive(paramLock)

spi_device_handle_t spi_reg, spi_fifo;

void cmt_spi3_init(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const int32_t spi_speed)
{
    paramLock = xSemaphoreCreateMutex();

    spi_host_device_t host_device;
    if (!SpiManagerInst.claim_bus(host_device))
        ESP_ERROR_CHECK(ESP_FAIL);

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_sdio,
        .miso_io_num = -1, // single wire MOSI/MISO
        .sclk_io_num = pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 32,
        .flags = 0,
        .intr_flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host_device, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg = {
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
        .pre_cb = nullptr,
        .post_cb = nullptr,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg, &spi_reg));

    // FiFo
    spi_device_interface_config_t devcfg2 = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 2,
        .cs_ena_posttrans = static_cast<uint8_t>(2 * spi_speed / 1000000), // >2 us
        .clock_speed_hz = spi_speed,
        .input_delay_ns = 0,
        .spics_io_num = pin_fcs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = nullptr,
        .post_cb = nullptr,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg2, &spi_fifo));
}

void cmt_spi3_write(const uint8_t addr, const uint8_t data)
{
    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = addr,
        .length = 8,
        .rxlength = 0,
        .user = nullptr,
        .tx_buffer = &data,
        .rx_buffer = nullptr,
    };
    SPI_PARAM_LOCK();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    SPI_PARAM_UNLOCK();
}

uint8_t cmt_spi3_read(const uint8_t addr)
{
    uint8_t data;
    spi_transaction_t t = {
        .flags = 0,
        .cmd = 1,
        .addr = addr,
        .length = 0,
        .rxlength = 8,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = &data,
    };
    SPI_PARAM_LOCK();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    SPI_PARAM_UNLOCK();
    return data;
}

void cmt_spi3_write_fifo(const uint8_t* buf, const uint16_t len)
{
    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 8,
        .rxlength = 0,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = nullptr,
    };

    SPI_PARAM_LOCK();
    spi_device_acquire_bus(spi_fifo, portMAX_DELAY);
    for (uint8_t i = 0; i < len; i++) {
        t.tx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
    }
    spi_device_release_bus(spi_fifo);
    SPI_PARAM_UNLOCK();
}

void cmt_spi3_read_fifo(uint8_t* buf, const uint16_t len)
{
    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = 0,
        .rxlength = 8,
        .user = nullptr,
        .tx_buffer = nullptr,
        .rx_buffer = nullptr,
    };

    SPI_PARAM_LOCK();
    spi_device_acquire_bus(spi_fifo, portMAX_DELAY);
    for (uint8_t i = 0; i < len; i++) {
        t.rx_buffer = buf + i;
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
    }
    spi_device_release_bus(spi_fifo);
    SPI_PARAM_UNLOCK();
}

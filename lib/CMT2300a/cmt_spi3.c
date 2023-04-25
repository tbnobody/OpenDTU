#include "cmt_spi3.h"
#include <Arduino.h>
#include <driver/spi_master.h>
#include <esp_rom_gpio.h> // for esp_rom_gpio_connect_out_signal

SemaphoreHandle_t paramLock=NULL;
#define SPI_PARAM_LOCK()    do {} while (xSemaphoreTake(paramLock, portMAX_DELAY) != pdPASS)
#define SPI_PARAM_UNLOCK()  xSemaphoreGive(paramLock)

spi_device_handle_t spi_reg, spi_fifo;

void cmt_spi3_init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, uint32_t spi_speed)
{
    paramLock = xSemaphoreCreateMutex();

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_sdio,
        .miso_io_num = -1, // single wire MOSI/MISO
        .sclk_io_num = pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .clock_speed_hz = spi_speed,
        .spics_io_num = pin_cs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, 0));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi_reg));

    // FiFo
    spi_device_interface_config_t devcfg2 = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // SPI mode 0
        .cs_ena_pretrans = 2,
        .cs_ena_posttrans = (uint8_t)(1 / (spi_speed * 10e6 * 2) + 2), // >2 us
        .clock_speed_hz = spi_speed,
        .spics_io_num = pin_fcs,
        .flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg2, &spi_fifo));

    esp_rom_gpio_connect_out_signal(pin_sdio, spi_periph_signal[SPI2_HOST].spid_out, true, false);
    delay(100);
}

void cmt_spi3_write(uint8_t addr, uint8_t dat)
{
    uint8_t tx_data[2];
    tx_data[0] = ~addr;
    tx_data[1] = ~dat;
    spi_transaction_t t = {
        .length = 2 * 8,
        .tx_buffer = &tx_data,
        .rx_buffer = NULL
    };
    SPI_PARAM_LOCK();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    SPI_PARAM_UNLOCK();
    delayMicroseconds(100);
}

uint8_t cmt_spi3_read(uint8_t addr)
{
    uint8_t tx_data, rx_data;
    tx_data = ~(addr | 0x80); // negation and MSB high (read command)
    spi_transaction_t t = {
        .length = 8,
        .rxlength = 8,
        .tx_buffer = &tx_data,
        .rx_buffer = &rx_data
    };
    SPI_PARAM_LOCK();
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi_reg, &t));
    SPI_PARAM_UNLOCK();
    delayMicroseconds(100);
    return rx_data;
}

void cmt_spi3_write_fifo(const uint8_t* buf, uint16_t len)
{
    uint8_t tx_data;

    spi_transaction_t t = {
        .flags = SPI_TRANS_MODE_OCT,
        .length = 8,
        .tx_buffer = &tx_data, // reference to write data
        .rx_buffer = NULL
    };

    SPI_PARAM_LOCK();
    for (uint8_t i = 0; i < len; i++) {
        tx_data = ~buf[i]; // negate buffer contents
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
    }
    SPI_PARAM_UNLOCK();
}

void cmt_spi3_read_fifo(uint8_t* buf, uint16_t len)
{
    uint8_t rx_data;

    spi_transaction_t t = {
        .length = 8,
        .rxlength = 8,
        .tx_buffer = NULL,
        .rx_buffer = &rx_data
    };

    SPI_PARAM_LOCK();
    for (uint8_t i = 0; i < len; i++) {
        ESP_ERROR_CHECK(spi_device_polling_transmit(spi_fifo, &t));
        delayMicroseconds(4); // > 4 us
        buf[i] = rx_data;
    }
    SPI_PARAM_UNLOCK();
}

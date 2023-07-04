#include "nrf_hal.h"

#define NRF_MAX_TRANSFER_SZ 64

nrf_hal::nrf_hal(gpio_num_t pin_mosi, gpio_num_t pin_miso, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_en) :
    pin_mosi(pin_mosi),
    pin_miso(pin_miso),
    pin_clk(pin_clk),
    pin_cs(pin_cs),
    pin_en(pin_en)
{

}

void nrf_hal::patch(spi_host_device_t host_device)
{
    gpio_hold_en(pin_cs);

    gpio_reset_pin(pin_cs);

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = NRF_MAX_TRANSFER_SZ,
        .flags = 0,
        .intr_flags = 0
    };
    ESP_ERROR_CHECK(spi_bus_initialize(host_device, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = 10000000,
        .input_delay_ns = 0,
        .spics_io_num = pin_cs,
        .flags = 0,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg, &spi));

    gpio_hold_dis(pin_cs);
}

void nrf_hal::unpatch(spi_host_device_t host_device)
{
    gpio_hold_en(pin_cs);

    ESP_ERROR_CHECK(spi_bus_remove_device(spi));
    ESP_ERROR_CHECK(spi_bus_free(host_device));

    gpio_reset_pin(pin_cs);
    gpio_set_direction(pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_cs, 1);

    gpio_hold_dis(pin_cs);
}

bool nrf_hal::begin()
{
    gpio_reset_pin(pin_cs);
    gpio_set_direction(pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_cs, 1);

    gpio_reset_pin(pin_en);
    gpio_set_direction(pin_en, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_en, 0);

    return true;
}

void nrf_hal::end()
{

}

void nrf_hal::ce(bool level)
{
    gpio_set_level(pin_en, level);
}

uint8_t nrf_hal::write(uint8_t cmd, const uint8_t* buf, uint8_t len)
{
    uint8_t data[NRF_MAX_TRANSFER_SZ];
    data[0] = cmd;
    for (size_t i = 0; i < len; ++i) {
        data[i + 1] = buf[i];
    }

    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = ((size_t)len + 1u) << 3,
        .rxlength = ((size_t)len + 1u) << 3,
        .user = NULL,
        .tx_buffer = data,
        .rx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));

    release_spi();

    return data[0]; // status
}

uint8_t nrf_hal::write(uint8_t cmd, const uint8_t* buf, uint8_t data_len, uint8_t blank_len)
{
    uint8_t data[NRF_MAX_TRANSFER_SZ];
    data[0] = cmd;
    for (size_t i = 0; i < data_len; ++i) {
        data[i + 1u] = buf[i];
    }
    for (size_t i = 0; i < blank_len; ++i) {
        data[i + data_len + 1u] = 0;
    }

    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = ((size_t)data_len + blank_len + 1u) << 3,
        .rxlength = ((size_t)data_len + blank_len + 1u) << 3,
        .user = NULL,
        .tx_buffer = data,
        .rx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));

    release_spi();

    return data[0]; // status
}

uint8_t nrf_hal::read(uint8_t cmd, uint8_t* buf, uint8_t len)
{
    uint8_t data[NRF_MAX_TRANSFER_SZ];
    data[0] = cmd;
    for (size_t i = 0; i < len; ++i) {
        data[i + 1u] = 0xff;
    }

    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = ((size_t)len + 1u) << 3,
        .rxlength = ((size_t)len + 1u) << 3,
        .user = NULL,
        .tx_buffer = data,
        .rx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));

    release_spi();

    for (size_t i = 0; i < len; ++i) {
        buf[i] = data[i + 1u];
    }
    return data[0]; // status
}

uint8_t nrf_hal::read(uint8_t cmd, uint8_t* buf, uint8_t data_len, uint8_t blank_len)
{
    uint8_t data[NRF_MAX_TRANSFER_SZ];
    data[0] = cmd;
    for (size_t i = 0; i < data_len; ++i) {
        data[i + 1u] = 0xff;
    }
    for (size_t i = 0; i < blank_len; ++i) {
        data[i + data_len + 1u] = 0xff;
    }

    request_spi();

    spi_transaction_t t = {
        .flags = 0,
        .cmd = 0,
        .addr = 0,
        .length = ((size_t)data_len + blank_len + 1u) << 3,
        .rxlength = ((size_t)data_len + blank_len + 1u) << 3,
        .user = NULL,
        .tx_buffer = data,
        .rx_buffer = data
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));

    release_spi();

    for (size_t i = 0; i < data_len; ++i) {
        buf[i] = data[i + 1u];
    }
    return data[0]; // status
}

#include "nrf_hal.h"

#include <esp_rom_gpio.h>

#define NRF_MAX_TRANSFER_SZ 64

nrf_hal::nrf_hal(gpio_num_t pin_mosi, gpio_num_t pin_miso, gpio_num_t pin_clk, gpio_num_t pin_cs, gpio_num_t pin_en) :
    pin_mosi(pin_mosi),
    pin_miso(pin_miso),
    pin_clk(pin_clk),
    pin_cs(pin_cs),
    pin_en(pin_en)
{

}

void nrf_hal::patch()
{
    esp_rom_gpio_connect_out_signal(pin_mosi, spi_periph_signal[host_device].spid_out, false, false);
    esp_rom_gpio_connect_in_signal(pin_miso, spi_periph_signal[host_device].spiq_in, false);
    esp_rom_gpio_connect_out_signal(pin_clk, spi_periph_signal[host_device].spiclk_out, false, false);
}

void nrf_hal::unpatch()
{
    esp_rom_gpio_connect_out_signal(pin_mosi, SIG_GPIO_OUT_IDX, false, false);
    esp_rom_gpio_connect_in_signal(pin_miso, GPIO_MATRIX_CONST_ZERO_INPUT, false);
    esp_rom_gpio_connect_out_signal(pin_clk, SIG_GPIO_OUT_IDX, false, false);
}

bool nrf_hal::begin()
{
    host_device = spi_patcher_inst.init();

    gpio_reset_pin(pin_mosi);
    gpio_set_direction(pin_mosi, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_mosi, 1);

    gpio_reset_pin(pin_miso);
    gpio_set_direction(pin_miso, GPIO_MODE_INPUT);

    gpio_reset_pin(pin_clk);
    gpio_set_direction(pin_clk, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_clk, 0);

    gpio_reset_pin(pin_cs);
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
        .pre_cb = nullptr,
        .post_cb = nullptr
    };
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &devcfg, &spi));

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

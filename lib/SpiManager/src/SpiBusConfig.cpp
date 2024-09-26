// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiBusConfig.h"

#include <driver/gpio.h>
#include <esp_rom_gpio.h>
#include <soc/spi_periph.h>

SpiBusConfig::SpiBusConfig(gpio_num_t _pin_mosi, gpio_num_t _pin_miso, gpio_num_t _pin_sclk)
    : pin_mosi(_pin_mosi)
    , pin_miso(_pin_miso)
    , pin_sclk(_pin_sclk)
{
    if (pin_mosi != GPIO_NUM_NC) {
        ESP_ERROR_CHECK(gpio_reset_pin(pin_mosi));
        ESP_ERROR_CHECK(gpio_set_direction(pin_mosi, GPIO_MODE_INPUT_OUTPUT));
    }

    if (pin_miso != GPIO_NUM_NC) {
        ESP_ERROR_CHECK(gpio_reset_pin(pin_miso));
        ESP_ERROR_CHECK(gpio_set_direction(pin_miso, GPIO_MODE_INPUT));
    }

    if (pin_sclk != GPIO_NUM_NC) {
        ESP_ERROR_CHECK(gpio_reset_pin(pin_sclk));
        ESP_ERROR_CHECK(gpio_set_direction(pin_sclk, GPIO_MODE_INPUT_OUTPUT));
    }
}

SpiBusConfig::~SpiBusConfig()
{
    if (pin_mosi != GPIO_NUM_NC)
        ESP_ERROR_CHECK(gpio_reset_pin(pin_mosi));

    if (pin_miso != GPIO_NUM_NC)
        ESP_ERROR_CHECK(gpio_reset_pin(pin_miso));

    if (pin_sclk != GPIO_NUM_NC)
        ESP_ERROR_CHECK(gpio_reset_pin(pin_sclk));
}

void SpiBusConfig::patch(spi_host_device_t host_device)
{
    if (pin_mosi != GPIO_NUM_NC) {
        esp_rom_gpio_connect_out_signal(pin_mosi, spi_periph_signal[host_device].spid_out, false, false);
        esp_rom_gpio_connect_in_signal(pin_mosi, spi_periph_signal[host_device].spid_in, false);
    }

    if (pin_miso != GPIO_NUM_NC)
        esp_rom_gpio_connect_in_signal(pin_miso, spi_periph_signal[host_device].spiq_in, false);

    if (pin_sclk != GPIO_NUM_NC) {
        esp_rom_gpio_connect_out_signal(pin_sclk, spi_periph_signal[host_device].spiclk_out, false, false);
        esp_rom_gpio_connect_in_signal(pin_sclk, spi_periph_signal[host_device].spiclk_in, false);
    }
}

void SpiBusConfig::unpatch(spi_host_device_t host_device)
{
    if (pin_mosi != GPIO_NUM_NC) {
        esp_rom_gpio_connect_out_signal(pin_mosi, SIG_GPIO_OUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, spi_periph_signal[host_device].spid_in, false);
    }

    if (pin_miso != GPIO_NUM_NC)
        esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, spi_periph_signal[host_device].spiq_in, false);

    if (pin_sclk != GPIO_NUM_NC) {
        esp_rom_gpio_connect_out_signal(pin_sclk, SIG_GPIO_OUT_IDX, false, false);
        esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, spi_periph_signal[host_device].spiclk_in, false);
    }
}

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ETH.h>
#include <stdint.h>

#define PINMAPPING_FILENAME "/pin_mapping.json"
#define PINMAPPING_LED_COUNT 2

#define MAPPING_NAME_STRLEN 31

struct PinMapping_t {
    char name[MAPPING_NAME_STRLEN + 1];

    gpio_num_t nrf24_miso;
    gpio_num_t nrf24_mosi;
    gpio_num_t nrf24_clk;
    gpio_num_t nrf24_irq;
    gpio_num_t nrf24_en;
    gpio_num_t nrf24_cs;

    gpio_num_t cmt_clk;
    gpio_num_t cmt_cs;
    gpio_num_t cmt_fcs;
    gpio_num_t cmt_gpio2;
    gpio_num_t cmt_gpio3;
    gpio_num_t cmt_sdio;

    gpio_num_t w5500_mosi;
    gpio_num_t w5500_miso;
    gpio_num_t w5500_sclk;
    gpio_num_t w5500_cs;
    gpio_num_t w5500_int;
    gpio_num_t w5500_rst;

#if CONFIG_ETH_USE_ESP32_EMAC
    int8_t eth_phy_addr;
    bool eth_enabled;
    gpio_num_t eth_power;
    gpio_num_t eth_mdc;
    gpio_num_t eth_mdio;
    eth_phy_type_t eth_type;
    eth_clock_mode_t eth_clk_mode;
#endif

    uint8_t display_type;
    gpio_num_t display_data;
    gpio_num_t display_clk;
    gpio_num_t display_cs;
    gpio_num_t display_reset;

    gpio_num_t led[PINMAPPING_LED_COUNT];
};

class PinMappingClass {
public:
    PinMappingClass();
    bool init(const String& deviceMapping);
    PinMapping_t& get();

    bool isMappingSelected() const { return _mappingSelected; }

    bool isValidNrf24Config() const;
    bool isValidCmt2300Config() const;
    bool isValidW5500Config() const;
#if CONFIG_ETH_USE_ESP32_EMAC
    bool isValidEthConfig() const;
#endif

private:
    PinMapping_t _pinMapping;

    bool _mappingSelected = false;
};

extern PinMappingClass PinMapping;

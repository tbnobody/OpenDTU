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
    int8_t nrf24_miso;
    int8_t nrf24_mosi;
    int8_t nrf24_clk;
    int8_t nrf24_irq;
    int8_t nrf24_en;
    int8_t nrf24_cs;

    int8_t cmt_clk;
    int8_t cmt_cs;
    int8_t cmt_fcs;
    int8_t cmt_gpio2;
    int8_t cmt_gpio3;
    int8_t cmt_sdio;

    int8_t w5500_sclk;
    int8_t w5500_mosi;
    int8_t w5500_miso;
    int8_t w5500_cs;
    int8_t w5500_int;
    int8_t w5500_rst;

    int8_t eth_phy_addr;
    bool eth_enabled;
    int eth_power;
    int eth_mdc;
    int eth_mdio;
    eth_phy_type_t eth_type;
    eth_clock_mode_t eth_clk_mode;
    uint8_t display_type;
    uint8_t display_data;
    uint8_t display_clk;
    uint8_t display_cs;
    uint8_t display_reset;
    int8_t led[PINMAPPING_LED_COUNT];

    // OpenDTU-OnBattery-specific pins below
    int8_t victron_tx;
    int8_t victron_rx;
    int8_t victron_tx2;
    int8_t victron_rx2;
    int8_t victron_tx3;
    int8_t victron_rx3;
    int8_t battery_rx;
    int8_t battery_rxen;
    int8_t battery_tx;
    int8_t battery_txen;
    int8_t huawei_miso;
    int8_t huawei_mosi;
    int8_t huawei_clk;
    int8_t huawei_irq;
    int8_t huawei_cs;
    int8_t huawei_power;
    int8_t powermeter_rx;
    int8_t powermeter_tx;
    int8_t powermeter_dere;
};

class PinMappingClass {
public:
    PinMappingClass();
    bool init(const String& deviceMapping);
    PinMapping_t& get();

    bool isValidNrf24Config() const;
    bool isValidCmt2300Config() const;
    bool isValidW5500Config() const;
    bool isValidEthConfig() const;
    bool isValidHuaweiConfig() const;

private:
    PinMapping_t _pinMapping;
};

extern PinMappingClass PinMapping;

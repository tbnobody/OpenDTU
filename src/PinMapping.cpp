// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "PinMapping.h"
#include "MessageOutput.h"
#include "Utils.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string.h>

#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE 0U
#endif

#ifndef DISPLAY_DATA
#define DISPLAY_DATA 255U
#endif

#ifndef DISPLAY_CLK
#define DISPLAY_CLK 255U
#endif

#ifndef DISPLAY_CS
#define DISPLAY_CS 255U
#endif

#ifndef DISPLAY_RESET
#define DISPLAY_RESET 255U
#endif

#ifndef LED0
#define LED0 -1
#endif

#ifndef LED1
#define LED1 -1
#endif

#ifndef HOYMILES_PIN_SCLK
#define HOYMILES_PIN_SCLK -1
#endif

#ifndef HOYMILES_PIN_CS
#define HOYMILES_PIN_CS -1
#endif

#ifndef HOYMILES_PIN_CE
#define HOYMILES_PIN_CE -1
#endif

#ifndef HOYMILES_PIN_IRQ
#define HOYMILES_PIN_IRQ -1
#endif

#ifndef HOYMILES_PIN_MISO
#define HOYMILES_PIN_MISO -1
#endif

#ifndef HOYMILES_PIN_MOSI
#define HOYMILES_PIN_MOSI -1
#endif

#ifndef CMT_CLK
#define CMT_CLK -1
#endif

#ifndef CMT_CS
#define CMT_CS -1
#endif

#ifndef CMT_FCS
#define CMT_FCS -1
#endif

#ifndef CMT_GPIO2
#define CMT_GPIO2 -1
#endif

#ifndef CMT_GPIO3
#define CMT_GPIO3 -1
#endif

#ifndef CMT_SDIO
#define CMT_SDIO -1
#endif

#ifndef W5500_MOSI
#define W5500_MOSI -1
#endif

#ifndef W5500_MISO
#define W5500_MISO -1
#endif

#ifndef W5500_SCLK
#define W5500_SCLK -1
#endif

#ifndef W5500_CS
#define W5500_CS -1
#endif

#ifndef W5500_INT
#define W5500_INT -1
#endif

#ifndef W5500_RST
#define W5500_RST -1
#endif

#if CONFIG_ETH_USE_ESP32_EMAC

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR -1
#endif

#ifndef ETH_PHY_POWER
#define ETH_PHY_POWER -1
#endif

#ifndef ETH_PHY_MDC
#define ETH_PHY_MDC -1
#endif

#ifndef ETH_PHY_MDIO
#define ETH_PHY_MDIO -1
#endif

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#endif

#ifndef ETH_CLK_MODE
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#endif

#endif

PinMappingClass PinMapping;

PinMappingClass::PinMappingClass()
{
    memset(&_pinMapping, 0x0, sizeof(_pinMapping));
    _pinMapping.nrf24_clk = HOYMILES_PIN_SCLK;
    _pinMapping.nrf24_cs = HOYMILES_PIN_CS;
    _pinMapping.nrf24_en = HOYMILES_PIN_CE;
    _pinMapping.nrf24_irq = HOYMILES_PIN_IRQ;
    _pinMapping.nrf24_miso = HOYMILES_PIN_MISO;
    _pinMapping.nrf24_mosi = HOYMILES_PIN_MOSI;

    _pinMapping.cmt_clk = CMT_CLK;
    _pinMapping.cmt_cs = CMT_CS;
    _pinMapping.cmt_fcs = CMT_FCS;
    _pinMapping.cmt_gpio2 = CMT_GPIO2;
    _pinMapping.cmt_gpio3 = CMT_GPIO3;
    _pinMapping.cmt_sdio = CMT_SDIO;

    _pinMapping.w5500_mosi = W5500_MOSI;
    _pinMapping.w5500_miso = W5500_MISO;
    _pinMapping.w5500_sclk = W5500_SCLK;
    _pinMapping.w5500_cs = W5500_CS;
    _pinMapping.w5500_int = W5500_INT;
    _pinMapping.w5500_rst = W5500_RST;

#if CONFIG_ETH_USE_ESP32_EMAC
#ifdef OPENDTU_ETHERNET
    _pinMapping.eth_enabled = true;
#else
    _pinMapping.eth_enabled = false;
#endif
    _pinMapping.eth_phy_addr = ETH_PHY_ADDR;
    _pinMapping.eth_power = ETH_PHY_POWER;
    _pinMapping.eth_mdc = ETH_PHY_MDC;
    _pinMapping.eth_mdio = ETH_PHY_MDIO;
    _pinMapping.eth_type = ETH_PHY_TYPE;
    _pinMapping.eth_clk_mode = ETH_CLK_MODE;
#endif

    _pinMapping.display_type = DISPLAY_TYPE;
    _pinMapping.display_data = DISPLAY_DATA;
    _pinMapping.display_clk = DISPLAY_CLK;
    _pinMapping.display_cs = DISPLAY_CS;
    _pinMapping.display_reset = DISPLAY_RESET;

    _pinMapping.led[0] = LED0;
    _pinMapping.led[1] = LED1;
}

PinMapping_t& PinMappingClass::get()
{
    return _pinMapping;
}

bool PinMappingClass::init(const String& deviceMapping)
{
    File f = LittleFS.open(PINMAPPING_FILENAME, "r", false);

    if (!f) {
        return false;
    }

    Utils::skipBom(f);

    JsonDocument doc;
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
        MessageOutput.println("Failed to read file, using default configuration");
    }

    for (uint8_t i = 0; i < doc.size(); i++) {
        String devName = doc[i]["name"] | "";
        if (devName == deviceMapping) {
            strlcpy(_pinMapping.name, devName.c_str(), sizeof(_pinMapping.name));
            _pinMapping.nrf24_clk = doc[i]["nrf24"]["clk"] | HOYMILES_PIN_SCLK;
            _pinMapping.nrf24_cs = doc[i]["nrf24"]["cs"] | HOYMILES_PIN_CS;
            _pinMapping.nrf24_en = doc[i]["nrf24"]["en"] | HOYMILES_PIN_CE;
            _pinMapping.nrf24_irq = doc[i]["nrf24"]["irq"] | HOYMILES_PIN_IRQ;
            _pinMapping.nrf24_miso = doc[i]["nrf24"]["miso"] | HOYMILES_PIN_MISO;
            _pinMapping.nrf24_mosi = doc[i]["nrf24"]["mosi"] | HOYMILES_PIN_MOSI;

            _pinMapping.cmt_clk = doc[i]["cmt"]["clk"] | CMT_CLK;
            _pinMapping.cmt_cs = doc[i]["cmt"]["cs"] | CMT_CS;
            _pinMapping.cmt_fcs = doc[i]["cmt"]["fcs"] | CMT_FCS;
            _pinMapping.cmt_gpio2 = doc[i]["cmt"]["gpio2"] | CMT_GPIO2;
            _pinMapping.cmt_gpio3 = doc[i]["cmt"]["gpio3"] | CMT_GPIO3;
            _pinMapping.cmt_sdio = doc[i]["cmt"]["sdio"] | CMT_SDIO;

            _pinMapping.w5500_mosi = doc[i]["w5500"]["mosi"] | W5500_MOSI;
            _pinMapping.w5500_miso = doc[i]["w5500"]["miso"] | W5500_MISO;
            _pinMapping.w5500_sclk = doc[i]["w5500"]["sclk"] | W5500_SCLK;
            _pinMapping.w5500_cs = doc[i]["w5500"]["cs"] | W5500_CS;
            _pinMapping.w5500_int = doc[i]["w5500"]["int"] | W5500_INT;
            _pinMapping.w5500_rst = doc[i]["w5500"]["rst"] | W5500_RST;

#if CONFIG_ETH_USE_ESP32_EMAC
#ifdef OPENDTU_ETHERNET
            _pinMapping.eth_enabled = doc[i]["eth"]["enabled"] | true;
#else
            _pinMapping.eth_enabled = doc[i]["eth"]["enabled"] | false;
#endif
            _pinMapping.eth_phy_addr = doc[i]["eth"]["phy_addr"] | ETH_PHY_ADDR;
            _pinMapping.eth_power = doc[i]["eth"]["power"] | ETH_PHY_POWER;
            _pinMapping.eth_mdc = doc[i]["eth"]["mdc"] | ETH_PHY_MDC;
            _pinMapping.eth_mdio = doc[i]["eth"]["mdio"] | ETH_PHY_MDIO;
            _pinMapping.eth_type = doc[i]["eth"]["type"] | ETH_PHY_TYPE;
            _pinMapping.eth_clk_mode = doc[i]["eth"]["clk_mode"] | ETH_CLK_MODE;
#endif

            _pinMapping.display_type = doc[i]["display"]["type"] | DISPLAY_TYPE;
            _pinMapping.display_data = doc[i]["display"]["data"] | DISPLAY_DATA;
            _pinMapping.display_clk = doc[i]["display"]["clk"] | DISPLAY_CLK;
            _pinMapping.display_cs = doc[i]["display"]["cs"] | DISPLAY_CS;
            _pinMapping.display_reset = doc[i]["display"]["reset"] | DISPLAY_RESET;

            _pinMapping.led[0] = doc[i]["led"]["led0"] | LED0;
            _pinMapping.led[1] = doc[i]["led"]["led1"] | LED1;

            return true;
        }
    }

    return false;
}

bool PinMappingClass::isValidNrf24Config() const
{
    return _pinMapping.nrf24_clk >= 0
        && _pinMapping.nrf24_cs >= 0
        && _pinMapping.nrf24_en >= 0
        && _pinMapping.nrf24_irq >= 0
        && _pinMapping.nrf24_miso >= 0
        && _pinMapping.nrf24_mosi >= 0;
}

bool PinMappingClass::isValidCmt2300Config() const
{
    return _pinMapping.cmt_clk >= 0
        && _pinMapping.cmt_cs >= 0
        && _pinMapping.cmt_fcs >= 0
        && _pinMapping.cmt_sdio >= 0;
}

bool PinMappingClass::isValidW5500Config() const
{
    return _pinMapping.w5500_mosi >= 0
        && _pinMapping.w5500_miso >= 0
        && _pinMapping.w5500_sclk >= 0
        && _pinMapping.w5500_cs >= 0
        && _pinMapping.w5500_int >= 0
        && _pinMapping.w5500_rst >= 0;
}

#if CONFIG_ETH_USE_ESP32_EMAC
bool PinMappingClass::isValidEthConfig() const
{
    return _pinMapping.eth_enabled
        && _pinMapping.eth_mdc >= 0
        && _pinMapping.eth_mdio >= 0;
}
#endif

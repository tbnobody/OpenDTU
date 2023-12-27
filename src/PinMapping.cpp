// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 - 2023 Thomas Basler and others
 */
#include "PinMapping.h"
#include "MessageOutput.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <string.h>

#define JSON_BUFFER_SIZE 6144

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

#ifndef VICTRON_PIN_TX
#define VICTRON_PIN_TX -1
#endif

#ifndef VICTRON_PIN_RX
#define VICTRON_PIN_RX -1
#endif

#ifndef BATTERY_PIN_RX
#define BATTERY_PIN_RX -1
#endif

#ifdef PYLONTECH_PIN_RX
#undef BATTERY_PIN_RX
#define BATTERY_PIN_RX PYLONTECH_PIN_RX
#endif

#ifndef BATTERY_PIN_RXEN
#define BATTERY_PIN_RXEN -1
#endif

#ifndef BATTERY_PIN_TX
#define BATTERY_PIN_TX -1
#endif

#ifdef PYLONTECH_PIN_TX
#undef BATTERY_PIN_TX
#define BATTERY_PIN_TX PYLONTECH_PIN_TX
#endif

#ifndef BATTERY_PIN_TXEN
#define BATTERY_PIN_TXEN -1
#endif

#ifndef HUAWEI_PIN_MISO
#define HUAWEI_PIN_MISO -1
#endif

#ifndef HUAWEI_PIN_MOSI
#define HUAWEI_PIN_MOSI -1
#endif

#ifndef HUAWEI_PIN_SCLK
#define HUAWEI_PIN_SCLK -1
#endif

#ifndef HUAWEI_PIN_CS
#define HUAWEI_PIN_CS -1
#endif

#ifndef HUAWEI_PIN_IRQ
#define HUAWEI_PIN_IRQ -1
#endif

#ifndef HUAWEI_PIN_POWER
#define HUAWEI_PIN_POWER -1
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

    _pinMapping.display_type = DISPLAY_TYPE;
    _pinMapping.display_data = DISPLAY_DATA;
    _pinMapping.display_clk = DISPLAY_CLK;
    _pinMapping.display_cs = DISPLAY_CS;
    _pinMapping.display_reset = DISPLAY_RESET;
   
    _pinMapping.victron_tx = VICTRON_PIN_TX;
    _pinMapping.victron_rx = VICTRON_PIN_RX;

    _pinMapping.battery_rx = BATTERY_PIN_RX;
    _pinMapping.battery_rxen = BATTERY_PIN_RXEN;
    _pinMapping.battery_tx = BATTERY_PIN_TX;
    _pinMapping.battery_txen = BATTERY_PIN_TXEN;

    _pinMapping.huawei_miso = HUAWEI_PIN_MISO;
    _pinMapping.huawei_mosi = HUAWEI_PIN_MOSI;
    _pinMapping.huawei_clk = HUAWEI_PIN_SCLK;
    _pinMapping.huawei_cs = HUAWEI_PIN_CS;
    _pinMapping.huawei_irq = HUAWEI_PIN_IRQ;
    _pinMapping.huawei_power = HUAWEI_PIN_POWER;
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

    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
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

            _pinMapping.display_type = doc[i]["display"]["type"] | DISPLAY_TYPE;
            _pinMapping.display_data = doc[i]["display"]["data"] | DISPLAY_DATA;
            _pinMapping.display_clk = doc[i]["display"]["clk"] | DISPLAY_CLK;
            _pinMapping.display_cs = doc[i]["display"]["cs"] | DISPLAY_CS;
            _pinMapping.display_reset = doc[i]["display"]["reset"] | DISPLAY_RESET;

            _pinMapping.victron_rx = doc[i]["victron"]["rx"] | VICTRON_PIN_RX;
            _pinMapping.victron_tx = doc[i]["victron"]["tx"] | VICTRON_PIN_TX;

            _pinMapping.battery_rx = doc[i]["battery"]["rx"] | BATTERY_PIN_RX;
            _pinMapping.battery_rxen = doc[i]["battery"]["rxen"] | BATTERY_PIN_RXEN;
            _pinMapping.battery_tx = doc[i]["battery"]["tx"] | BATTERY_PIN_TX;
            _pinMapping.battery_txen = doc[i]["battery"]["txen"] | BATTERY_PIN_TXEN;

            _pinMapping.huawei_miso = doc[i]["huawei"]["miso"] | HUAWEI_PIN_MISO;
            _pinMapping.huawei_mosi = doc[i]["huawei"]["mosi"] | HUAWEI_PIN_MOSI;
            _pinMapping.huawei_clk = doc[i]["huawei"]["clk"] | HUAWEI_PIN_SCLK;
            _pinMapping.huawei_irq = doc[i]["huawei"]["irq"] | HUAWEI_PIN_IRQ;
            _pinMapping.huawei_cs = doc[i]["huawei"]["cs"] | HUAWEI_PIN_CS;
            _pinMapping.huawei_power = doc[i]["huawei"]["power"] | HUAWEI_PIN_POWER;

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

bool PinMappingClass::isValidEthConfig() const
{
    return _pinMapping.eth_enabled;
}

bool PinMappingClass::isValidHuaweiConfig()
{
    return _pinMapping.huawei_miso >= 0
        && _pinMapping.huawei_mosi >= 0
        && _pinMapping.huawei_clk >= 0
        && _pinMapping.huawei_irq >= 0
        && _pinMapping.huawei_cs >= 0
        && _pinMapping.huawei_power >= 0;
}

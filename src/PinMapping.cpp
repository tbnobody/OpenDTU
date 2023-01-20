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
        MessageOutput.println(F("Failed to read file, using default configuration"));
    }

    for (uint8_t i = 1; i <= doc.size(); i++) {
        String devName = doc[i]["name"] | "";
        if (devName == deviceMapping) {
            strlcpy(_pinMapping.name, devName.c_str(), sizeof(_pinMapping.name));
            _pinMapping.nrf24_clk = doc[i]["nrf24"]["clk"] | HOYMILES_PIN_SCLK;
            _pinMapping.nrf24_cs = doc[i]["nrf24"]["cs"] | HOYMILES_PIN_CS;
            _pinMapping.nrf24_en = doc[i]["nrf24"]["en"] | HOYMILES_PIN_CE;
            _pinMapping.nrf24_irq = doc[i]["nrf24"]["irq"] | HOYMILES_PIN_IRQ;
            _pinMapping.nrf24_miso = doc[i]["nrf24"]["miso"] | HOYMILES_PIN_MISO;
            _pinMapping.nrf24_mosi = doc[i]["nrf24"]["mosi"] | HOYMILES_PIN_MOSI;

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

            return true;
        }
    }

    return false;
}

bool PinMappingClass::isValidNrf24Config()
{
    return _pinMapping.nrf24_clk > 0
        && _pinMapping.nrf24_cs > 0
        && _pinMapping.nrf24_en > 0
        && _pinMapping.nrf24_irq > 0
        && _pinMapping.nrf24_miso > 0
        && _pinMapping.nrf24_mosi > 0;
}

bool PinMappingClass::isValidEthConfig()
{
    return _pinMapping.eth_enabled;
}
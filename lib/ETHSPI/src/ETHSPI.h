//-----------------------------------------------------------------------------
// 2024 Ahoy, https://ahoydtu.de
// adapted to OpenDTU-OnBattery
// Creative Commons - https://creativecommons.org/licenses/by-nc-sa/4.0/deed
//-----------------------------------------------------------------------------

#pragma once

#include <Arduino.h>
#include <esp_netif.h>
#include <driver/spi_master.h>

class ETHSPIClass
{
private:
    esp_eth_handle_t eth_handle;
    esp_netif_t *eth_netif;

public:
    ETHSPIClass();

    void begin(int8_t pin_sclk, int8_t pin_mosi, int8_t pin_miso, int8_t pin_cs, int8_t pin_int, int8_t pin_rst, spi_host_device_t host_id);
    String macAddress();
};

extern ETHSPIClass ETHSPI;

#pragma once

#include <Arduino.h>
//#include <esp_eth.h>

class ETHSPIClass
{
private:
    esp_eth_handle_t eth_handle;
    esp_netif_t *eth_netif;

public:
    ETHSPIClass();
    
    void begin(int8_t pin_sclk, int8_t pin_mosi, int8_t pin_miso, int8_t pin_cs, int8_t pin_int, int8_t pin_rst);
    String macAddress();
};

extern ETHSPIClass ETHSPI;

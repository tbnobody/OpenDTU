#pragma once

#include <esp_netif.h>

#include <Arduino.h>

class W5500 {
public:
    explicit W5500(int8_t pin_mosi, int8_t pin_miso, int8_t pin_sclk, int8_t pin_cs, int8_t pin_int, int8_t pin_rst);
    W5500(const W5500&) = delete;
    W5500 &operator=(const W5500&) = delete;
    ~W5500();

    String macAddress();

private:
    esp_eth_handle_t eth_handle;
    esp_netif_t *eth_netif;
};

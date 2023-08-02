#include "ETHSPI.h"

#include <driver/spi_master.h>

// Functions from WiFiGeneric
void tcpipInit();
void add_esp_interface_netif(esp_interface_t interface, esp_netif_t* esp_netif);

ETHSPIClass::ETHSPIClass() :
    eth_handle(nullptr),
    eth_netif(nullptr)
{

}

void ETHSPIClass::begin(int8_t pin_sclk, int8_t pin_mosi, int8_t pin_miso, int8_t pin_cs, int8_t pin_int, int8_t pin_rst)
{
    gpio_reset_pin(static_cast<gpio_num_t>(pin_rst));
    gpio_set_direction(static_cast<gpio_num_t>(pin_rst), GPIO_MODE_OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(pin_rst), 0);

    gpio_reset_pin(static_cast<gpio_num_t>(pin_sclk));
    gpio_reset_pin(static_cast<gpio_num_t>(pin_mosi));
    gpio_reset_pin(static_cast<gpio_num_t>(pin_miso));
    gpio_reset_pin(static_cast<gpio_num_t>(pin_cs));
    gpio_set_pull_mode(static_cast<gpio_num_t>(pin_miso), GPIO_PULLUP_ONLY);

    // Workaround, because calling gpio_install_isr_service directly causes issues with attachInterrupt later
    attachInterrupt(digitalPinToInterrupt(pin_int), nullptr, CHANGE);
    detachInterrupt(digitalPinToInterrupt(pin_int));
    gpio_reset_pin(static_cast<gpio_num_t>(pin_int));
    gpio_set_pull_mode(static_cast<gpio_num_t>(pin_int), GPIO_PULLUP_ONLY);

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 0, // uses default value internally
        .flags = 0,
        .intr_flags = 0
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .command_bits = 16, // actually address phase
        .address_bits = 8, // actually command phase
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0, // only 0 supported
        .cs_ena_posttrans = 0, // only 0 supported
        .clock_speed_hz = 20000000, // stable with on OpenDTU Fusion Shield
        .input_delay_ns = 0,
        .spics_io_num = pin_cs,
        .flags = 0,
        .queue_size = 20,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    spi_device_handle_t spi;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &spi));

    // Reset sequence
    delayMicroseconds(500);
    gpio_set_level(static_cast<gpio_num_t>(pin_rst), 1);
    delayMicroseconds(1000);

    // Arduino function to start networking stack if not already started
    tcpipInit();

    ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers()); // ?

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi);
    w5500_config.int_gpio_num = pin_int;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.rx_task_stack_size = 4096;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.reset_gpio_num = -1;
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

    // Configure MAC address
    uint8_t mac_addr[6];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac_addr));
    mac_addr[5] |= 0x03; // derive ethernet MAC address from base MAC address
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&netif_config);

    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    // Add to Arduino
    add_esp_interface_netif(ESP_IF_ETH, eth_netif);

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

String ETHSPIClass::macAddress()
{
    uint8_t mac_addr[6] = {0, 0, 0, 0, 0, 0};
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    char mac_addr_str[24];
    snprintf(mac_addr_str, sizeof(mac_addr_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return String(mac_addr_str);
}

ETHSPIClass ETHSPI;

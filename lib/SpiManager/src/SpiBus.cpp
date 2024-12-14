// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiBus.h"
#include "SpiBusConfig.h"
#include "SpiCallback.h"

SpiBus::SpiBus(const std::string& _id, spi_host_device_t _host_device)
    : id(_id)
    , host_device(_host_device)
    , cur_config(nullptr)
{
    spi_bus_config_t bus_config {
        .mosi_io_num = -1,
        .miso_io_num = -1,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 0, // defaults to SPI_MAX_DMA_LEN (=4092) or SOC_SPI_MAXIMUM_BUFFER_SIZE (=64)
        .flags = 0,
        .intr_flags = 0
    };

#if !CONFIG_IDF_TARGET_ESP32S2
    spi_dma_chan_t dma_channel = SPI_DMA_CH_AUTO;
#else
    // DMA for SPI3 on ESP32-S2 is shared with ADC/DAC, so we cannot use it here
    spi_dma_chan_t dma_channel = (host_device != SPI3_HOST ? SPI_DMA_CH_AUTO : SPI_DMA_DISABLED);
#endif

    ESP_ERROR_CHECK(spi_bus_initialize(host_device, &bus_config, dma_channel));
}

SpiBus::~SpiBus()
{
    ESP_ERROR_CHECK(spi_bus_free(host_device));
}

spi_device_handle_t SpiBus::add_device(const std::shared_ptr<SpiBusConfig>& bus_config, spi_device_interface_config_t& device_config)
{
    if (!SpiCallback::patch(shared_from_this(), bus_config, device_config))
        return nullptr;

    spi_device_handle_t device;
    ESP_ERROR_CHECK(spi_bus_add_device(host_device, &device_config, &device));
    return device;
}

// TODO: add remove_device (with spi_device_acquire_bus)

void SpiBus::apply_config(SpiBusConfig* config)
{
    if (cur_config)
        cur_config->unpatch(host_device);
    cur_config = config;
    if (cur_config)
        cur_config->patch(host_device);
}

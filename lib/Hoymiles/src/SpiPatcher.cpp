#include "SpiPatcher.h"

#define HOYMILES_HOST_DEVICE SPI2_HOST

SpiPatcher HoymilesSpiPatcher(HOYMILES_HOST_DEVICE);

SpiPatcher::SpiPatcher(spi_host_device_t host_device) :
    host_device(host_device),
    initialized(false),
    cur_handle(nullptr)
{
    // Use binary semaphore instead of mutex for performance reasons
    mutex = xSemaphoreCreateBinaryStatic(&mutex_buffer);
    xSemaphoreGive(mutex);
}

SpiPatcher::~SpiPatcher()
{
    vSemaphoreDelete(mutex);
}

spi_host_device_t SpiPatcher::init()
{
    if (!initialized) {
        initialized = true;

        spi_bus_config_t buscfg = {
            .mosi_io_num = -1,
            .miso_io_num = -1,
            .sclk_io_num = -1,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE,
            .flags = 0,
            .intr_flags = 0
        };
        ESP_ERROR_CHECK(spi_bus_initialize(host_device, &buscfg, SPI_DMA_DISABLED));
    }

    return host_device;
}

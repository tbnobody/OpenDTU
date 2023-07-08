#include "spi_patcher.h"

#define HOST_DEVICE SPI2_HOST

spi_patcher spi_patcher_inst(HOST_DEVICE);

spi_patcher::spi_patcher(spi_host_device_t host_device) :
    host_device(host_device),
    cur_handle(nullptr)
{
    // Use binary semaphore instead of mutex for performance reasons
    mutex = xSemaphoreCreateBinaryStatic(&mutex_buffer);
    xSemaphoreGive(mutex);
}

spi_patcher::~spi_patcher()
{
    vSemaphoreDelete(mutex);
}

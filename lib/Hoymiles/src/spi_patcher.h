#pragma once

#include "spi_patcher_handle.h"

#include <freertos/semphr.h>

class spi_patcher
{
public:
    explicit spi_patcher(spi_host_device_t host_device);
    ~spi_patcher();

    spi_host_device_t init();

    inline void request(spi_patcher_handle* handle)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (cur_handle != handle) {
            if (cur_handle) {
                cur_handle->unpatch();
            }
            cur_handle = handle;
            if (cur_handle) {
                cur_handle->patch();
            }
        }
    }

    inline void release()
    {
        xSemaphoreGive(mutex);
    }

private:
    const spi_host_device_t host_device;
    bool initialized;

    spi_patcher_handle* cur_handle;

    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
};

extern spi_patcher spi_patcher_inst;

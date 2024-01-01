#pragma once

#include "SpiPatcherHandle.h"

#include <driver/spi_master.h>
#include <freertos/semphr.h>

class SpiPatcher
{
public:
    explicit SpiPatcher(spi_host_device_t host_device);
    ~SpiPatcher();

    spi_host_device_t init();

    inline void request(SpiPatcherHandle* handle)
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

    SpiPatcherHandle* cur_handle;

    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
};

extern SpiPatcher HoymilesSpiPatcher;

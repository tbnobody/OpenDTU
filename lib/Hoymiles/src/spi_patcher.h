#pragma once

#include "spi_patcher_handle.h"

#include <freertos/semphr.h>

#include <Arduino.h>

class spi_patcher
{
public:
    spi_patcher(spi_host_device_t host_device);
    ~spi_patcher();

    inline void request(spi_patcher_handle* handle)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);

        if (cur_handle != handle) {
            if (cur_handle) {
                cur_handle->unpatch(host_device);
            }
            cur_handle = handle;
            if (cur_handle) {
                cur_handle->patch(host_device);
            }
        }
    }

    inline void release()
    {
        xSemaphoreGive(mutex);
    }

private:
    const spi_host_device_t host_device;
    spi_patcher_handle* cur_handle;

    SemaphoreHandle_t mutex;
    StaticSemaphore_t mutex_buffer;
};

extern spi_patcher spi_patcher_inst;

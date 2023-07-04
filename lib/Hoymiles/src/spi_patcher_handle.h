#pragma once

#include <driver/spi_master.h>

class spi_patcher_handle
{
public:
    spi_patcher_handle();
    virtual ~spi_patcher_handle();

    virtual void patch(spi_host_device_t host_device) = 0;
    virtual void unpatch(spi_host_device_t host_device) = 0;
};

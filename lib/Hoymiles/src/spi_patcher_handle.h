#pragma once

#include <driver/spi_master.h>

class spi_patcher_handle
{
public:
    spi_patcher_handle();
    virtual ~spi_patcher_handle();

    virtual void patch() = 0;
    virtual void unpatch() = 0;
};

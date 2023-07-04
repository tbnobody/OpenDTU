#pragma once

#ifdef __cplusplus

#include "spi_patcher_handle.h"

class cmt_spi_patcher_handle : public spi_patcher_handle
{
public:
    void patch(spi_host_device_t host_device) override;
    void unpatch(spi_host_device_t host_device) override;
};

extern "C"
{
    void cmt_request_spi();
    void cmt_release_spi();
}

#else

void cmt_request_spi(void);
void cmt_release_spi(void);

#endif

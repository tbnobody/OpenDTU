#include "cmt_spi_patcher_handle.h"

#include "cmt_spi3.h"

#include "spi_patcher.h"

cmt_spi_patcher_handle cmt_spi_patcher_handle_inst;

void cmt_spi_patcher_handle::patch(spi_host_device_t host_device)
{
    cmt_patch_spi(host_device);
}

void cmt_spi_patcher_handle::unpatch(spi_host_device_t host_device)
{
    cmt_unpatch_spi(host_device);
}

void cmt_request_spi()
{
    spi_patcher_inst.request(&cmt_spi_patcher_handle_inst);
}

void cmt_release_spi()
{
    spi_patcher_inst.release();
}

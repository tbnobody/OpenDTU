// SPDX-License-Identifier: GPL-2.0-or-later
#include "SpiCallback.h"

#include "SpiBus.h"
#include <array>
#include <optional>

namespace SpiCallback {
namespace {
    struct CallbackData {
        std::shared_ptr<SpiBus> bus;
        std::shared_ptr<SpiBusConfig> config;
        transaction_cb_t inner_pre_cb;
        transaction_cb_t inner_post_cb;
    };

    std::array<std::optional<CallbackData>, SPI_MANAGER_CALLBACK_COUNT> instances;

    template <int N>
    void IRAM_ATTR fn_pre_cb(spi_transaction_t* trans)
    {
        instances[N]->bus->require_config(instances[N]->config.get());
        if (instances[N]->inner_pre_cb)
            instances[N]->inner_pre_cb(trans);
    }

    template <int N>
    void IRAM_ATTR fn_post_cb(spi_transaction_t* trans)
    {
        if (instances[N]->inner_post_cb)
            instances[N]->inner_post_cb(trans);
    }

    template <int N>
    inline __attribute__((always_inline)) bool alloc(CallbackData*& instance, transaction_cb_t& pre_cb, transaction_cb_t& post_cb)
    {
        if constexpr (N > 0) {
            if (alloc<N - 1>(instance, pre_cb, post_cb))
                return true;
            if (!instances[N - 1]) {
                instances[N - 1].emplace();
                instance = &*instances[N - 1];
                pre_cb = fn_pre_cb<N - 1>;
                post_cb = fn_post_cb<N - 1>;
                return true;
            }
        }
        return false;
    }
}

bool patch(const std::shared_ptr<SpiBus>& bus, const std::shared_ptr<SpiBusConfig>& bus_config, spi_device_interface_config_t& device_config)
{
    CallbackData* instance;
    transaction_cb_t pre_cb;
    transaction_cb_t post_cb;
    if (!alloc<SPI_MANAGER_CALLBACK_COUNT>(instance, pre_cb, post_cb))
        return false;

    instance->bus = bus;
    instance->config = bus_config;
    instance->inner_pre_cb = device_config.pre_cb;
    instance->inner_post_cb = device_config.post_cb;
    device_config.pre_cb = pre_cb;
    device_config.post_cb = post_cb;

    return true;
}
}

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <SPI.h>
#include <mcp_can.h>
#include <SpiManager.h>
#include <gridcharger/huawei/HardwareInterface.h>

namespace GridChargers::Huawei {

class MCP2515 : public HardwareInterface {
public:
    ~MCP2515() override;

    bool init() final;

    bool sendMessage(uint32_t canId, std::array<uint8_t, 8> const& data) final;

private:
    // this is static because we cannot give back the bus once we claimed it.
    // as we are going to use a shared host/bus in the future, we won't use a
    // workaround for the limited time we use it like this.
    static std::optional<uint8_t> _oSpiBus;

    std::unique_ptr<SPIClass> _upSPI;
    std::unique_ptr<MCP_CAN> _upCAN;
    gpio_num_t _huaweiIrq; // IRQ pin

    // interlock access to the MCP2515 driver, as
    // thread-safety is not guaranteed by the MCP2515 driver.
    std::mutex _mutex;

    std::atomic<bool> _queueingTaskDone = false;
    std::atomic<bool> _stopQueueing = false;

    static void queueMessages(void* context);
};

} // namespace GridChargers::Huawei

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include <gridcharger/huawei/MCP2515.h>
#include "PinMapping.h"
#include "Configuration.h"
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "MCP2515";

namespace GridChargers::Huawei {

TaskHandle_t sIsrTaskHandle = nullptr;

void mcp2515Isr()
{
    if (sIsrTaskHandle == nullptr) { return; }
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(sIsrTaskHandle, &xHigherPriorityTaskWoken);
    // make sure that the high-priority hardware interface task is scheduled,
    // as the timing is very critical. CAN messages will be missed if the
    // MCP2515 interrupt is not serviced immediately, as a new message
    // overwrites a pending message.
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

std::optional<uint8_t> MCP2515::_oSpiBus = std::nullopt;

MCP2515::~MCP2515()
{
    detachInterrupt(digitalPinToInterrupt(_huaweiIrq));

    if (sIsrTaskHandle != nullptr) {
        _queueingTaskDone = false;
        _stopQueueing = true;

        while (!_queueingTaskDone) { delay(10); }
    }

    stopLoop();
    _upCAN.reset(nullptr);
    if (_upSPI) { _upSPI->end(); } // nullptr if init failed or never called
    _upSPI.reset(nullptr);
}

bool MCP2515::init()
{
    const PinMapping_t& pin = PinMapping.get();

    DTU_LOGI("clk = %d, miso = %d, mosi = %d, cs = %d, irq = %d",
            pin.huawei_clk, pin.huawei_miso, pin.huawei_mosi, pin.huawei_cs, pin.huawei_irq);

    if (pin.huawei_clk <= GPIO_NUM_NC || pin.huawei_miso <= GPIO_NUM_NC ||
        pin.huawei_mosi <= GPIO_NUM_NC || pin.huawei_cs <= GPIO_NUM_NC ||
        pin.huawei_irq <= GPIO_NUM_NC) {
        DTU_LOGE("invalid pin config");
        return false;
    }

    if (!_oSpiBus) {
        _oSpiBus = SpiManagerInst.claim_bus_arduino();
    }

    if (!_oSpiBus) {
        DTU_LOGE("no SPI host available");
        return false;
    }

    _upSPI = std::make_unique<SPIClass>(*_oSpiBus);

    _upSPI->begin(pin.huawei_clk, pin.huawei_miso, pin.huawei_mosi, pin.huawei_cs);
    pinMode(pin.huawei_cs, OUTPUT);
    digitalWrite(pin.huawei_cs, HIGH);

    auto mcp_frequency = MCP_8MHZ;
    auto frequency = Configuration.get().GridCharger.Can.Controller_Frequency;
    if (16000000UL == frequency) { mcp_frequency = MCP_16MHZ; }
    else if (8000000UL != frequency) {
        DTU_LOGW("unknown frequency %d Hz, using 8 MHz", mcp_frequency);
    }

    _upCAN = std::make_unique<MCP_CAN>(_upSPI.get(), pin.huawei_cs);
    if (_upCAN->begin(MCP_STDEXT, CAN_125KBPS, mcp_frequency) != CAN_OK) {
        DTU_LOGE("mcp_can begin() failed");
        return false;
    }

    const uint32_t myMask = 0xFFFF0000;      // filter for the first two bytes...
    const uint32_t myFilter = 0x10810000;    // ...with this value
    _upCAN->init_Mask(0, 1, myMask);
    _upCAN->init_Filt(0, 1, myFilter);
    _upCAN->init_Mask(1, 1, myMask);

    // Change to normal mode to allow messages to be transmitted
    _upCAN->setMode(MCP_NORMAL);

    if (!startLoop()) {
        DTU_LOGE("failed to start loop task");
        return false;
    }

    if (sIsrTaskHandle != nullptr) {
        // make the ISR aware of multiple instances if multiple instances of
        // this driver should be able to co-exist. only one is supported now.
        DTU_LOGE("ISR task handle already in use");
        stopLoop();
        return false;
    }

    uint32_t constexpr stackSize = 2048;
    if (pdPASS != xTaskCreate(MCP2515::queueMessages,
            "HuaweiMCP2515", stackSize, this, 20/*prio*/, &sIsrTaskHandle)) {
        DTU_LOGE("failed to create queueing task");
        stopLoop();
        return false;
    }

    _huaweiIrq = pin.huawei_irq;
    pinMode(_huaweiIrq, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_huaweiIrq), mcp2515Isr, FALLING);

    return true;
}

void MCP2515::queueMessages(void* context)
{
    auto& instance = *static_cast<MCP2515*>(context);

    INT32U rxId;
    unsigned char len = 0;
    unsigned char rxBuf[8];

    while (!instance._stopQueueing) {
        static auto constexpr resetNotificationValue = pdTRUE;
        static auto constexpr notificationTimeout = pdMS_TO_TICKS(500);

        ulTaskNotifyTake(resetNotificationValue, notificationTimeout);

        while (!digitalRead(instance._huaweiIrq)) {
            {
                std::lock_guard<std::mutex> lock(instance._mutex);
                instance._upCAN->readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
            }

            // Determine if ID is standard (11 bits) or extended (29 bits)
            if ((rxId & 0x80000000) != 0x80000000) { continue; }

            if (len != 8) { continue; }

            HardwareInterface::can_message_t msg;
            msg.canId = rxId & 0x1FFFFFFF; // mask piggy-backed MCP2515 bits
            msg.valueId = rxBuf[0] << 24 | rxBuf[1] << 16 | rxBuf[2] << 8 | rxBuf[3];
            msg.value = rxBuf[4] << 24 | rxBuf[5] << 16 | rxBuf[6] << 8 | rxBuf[7];

            instance.enqueueReceivedMessage(msg);
        }
    }

    instance._queueingTaskDone = true;
    sIsrTaskHandle = nullptr;

    vTaskDelete(nullptr);
}

bool MCP2515::sendMessage(uint32_t canId, std::array<uint8_t, 8> const& data)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_upCAN) { return false; }

    // MCP2515 CAN library requires a non-const pointer to the data
    uint8_t rwData[8];
    memcpy(rwData, data.data(), data.size());
    return _upCAN->sendMsgBuf(canId, 1, 8, rwData) == CAN_OK;
}

} // namespace GridChargers::Huawei

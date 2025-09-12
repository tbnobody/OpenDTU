// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Malte Schmidt and others
 */
#include <gridcharger/huawei/TWAI.h>
#include "PinMapping.h"
#include <driver/twai.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "TWAI";

namespace GridChargers::Huawei {

TWAI::~TWAI()
{
    if (_pollingTaskHandle != nullptr) {
        _pollingTaskDone = false;
        _stopPolling = true;

        while (!_pollingTaskDone) { delay(10); }

        _pollingTaskHandle = nullptr;
    }

    stopLoop();

    if (twai_stop() != ESP_OK) {
        DTU_LOGW("failed to stop driver");
        return;
    }

    if (twai_driver_uninstall() != ESP_OK) {
        DTU_LOGW("failed to uninstall driver");
    }

    DTU_LOGI("driver stopped and uninstalled");
}

bool TWAI::init()
{
    const PinMapping_t& pin = PinMapping.get();

    DTU_LOGI("rx = %d, tx = %d", pin.huawei_rx, pin.huawei_tx);

    if (pin.huawei_rx <= GPIO_NUM_NC || pin.huawei_tx <= GPIO_NUM_NC) {
        DTU_LOGE("invalid pin config");
        return false;
    }

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(pin.huawei_tx, pin.huawei_rx, TWAI_MODE_NORMAL);

    // interrupts at level 1 are in high demand, at least on ESP32-S3 boards,
    // but only a limited amount can be allocated. failing to allocate an
    // interrupt in the TWAI driver will cause a bootloop. we therefore
    // register the TWAI driver's interrupt at level 2. level 2 interrupts
    // should be available -- we don't really know. we would love to have the
    // esp_intr_dump() function, but that's not available yet in our version
    // of the underlying esp-idf.
    g_config.intr_flags = ESP_INTR_FLAG_LEVEL2;

    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        DTU_LOGE("Failed to install driver");
        return false;
    }

    if (twai_start() != ESP_OK) {
        DTU_LOGE("Failed to start driver");
        return false;
    }

    if (!startLoop()) {
        DTU_LOGE("failed to start loop task");
        return false;
    }

    // enable alert on message received
    uint32_t alertsToEnable = TWAI_ALERT_RX_DATA;
    if (twai_reconfigure_alerts(alertsToEnable, NULL) != ESP_OK) {
        DTU_LOGE("Failed to configure alerts");
        return false;
    }

    uint32_t constexpr stackSize = 2048;
    if (pdPASS != xTaskCreate(TWAI::pollAlerts,
            "HuaweiTwai", stackSize, this, 20/*prio*/, &_pollingTaskHandle)) {
        DTU_LOGE("failed to create polling task");
        stopLoop();
        return false;
    }

    DTU_LOGI("driver ready");

    return true;
}

void TWAI::pollAlerts(void* context)
{
    auto& instance = *static_cast<TWAI*>(context);
    uint32_t alerts;

    while (!instance._stopPolling) {
        // blocks until an alert was received or the timeout expired.
        if (twai_read_alerts(&alerts, pdMS_TO_TICKS(500)) != ESP_OK) { continue; }

        if ((alerts & TWAI_ALERT_RX_DATA) == 0) { continue; }

        while (true) {
            twai_message_t rxMessage;

            if (twai_receive(&rxMessage, pdMS_TO_TICKS(1)) != ESP_OK) { break; }

            if (rxMessage.extd != 1) { continue; } // we only process extended format messages

            if (rxMessage.data_length_code != 8) { continue; }

            HardwareInterface::can_message_t msg;
            msg.canId = rxMessage.identifier;
            msg.valueId = rxMessage.data[0] << 24 | rxMessage.data[1] << 16 | rxMessage.data[2] << 8 | rxMessage.data[3];
            msg.value = rxMessage.data[4] << 24 | rxMessage.data[5] << 16 | rxMessage.data[6] << 8 | rxMessage.data[7];

            instance.enqueueReceivedMessage(msg);
        }
    }

    instance._pollingTaskDone = true;

    vTaskDelete(nullptr);
}

bool TWAI::sendMessage(uint32_t canId, std::array<uint8_t, 8> const& data)
{
    twai_message_t txMsg;
    memset(&txMsg, 0, sizeof(txMsg));
    memcpy(txMsg.data, data.data(), data.size());
    txMsg.extd = 1;
    txMsg.data_length_code = data.size();
    txMsg.identifier = canId;

    return twai_transmit(&txMsg, pdMS_TO_TICKS(1000)) == ESP_OK;
}

} // namespace GridChargers::Huawei

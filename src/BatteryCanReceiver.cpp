// SPDX-License-Identifier: GPL-2.0-or-later
#include "BatteryCanReceiver.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include <driver/twai.h>

bool BatteryCanReceiver::init(bool verboseLogging, char const* providerName)
{
    _verboseLogging = verboseLogging;
    _providerName = providerName;

    MessageOutput.printf("[%s] Initialize interface...\r\n",
            _providerName);

    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.printf("[%s] Interface rx = %d, tx = %d\r\n",
            _providerName, pin.battery_rx, pin.battery_tx);

    if (pin.battery_rx < 0 || pin.battery_tx < 0) {
        MessageOutput.printf("[%s] Invalid pin config\r\n",
                _providerName);
        return false;
    }

    auto tx = static_cast<gpio_num_t>(pin.battery_tx);
    auto rx = static_cast<gpio_num_t>(pin.battery_rx);
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx, rx, TWAI_MODE_NORMAL);

    // interrupts at level 1 are in high demand, at least on ESP32-S3 boards,
    // but only a limited amount can be allocated. failing to allocate an
    // interrupt in the TWAI driver will cause a bootloop. we therefore
    // register the TWAI driver's interrupt at level 2. level 2 interrupts
    // should be available -- we don't really know. we would love to have the
    // esp_intr_dump() function, but that's not available yet in our version
    // of the underlying esp-idf.
    g_config.intr_flags = ESP_INTR_FLAG_LEVEL2;

    // Initialize configuration structures using macro initializers
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    esp_err_t twaiLastResult = twai_driver_install(&g_config, &t_config, &f_config);
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.printf("[%s] Twai driver installed\r\n",
                    _providerName);
            break;
        case ESP_ERR_INVALID_ARG:
            MessageOutput.printf("[%s] Twai driver install - invalid arg\r\n",
                    _providerName);
            return false;
            break;
        case ESP_ERR_NO_MEM:
            MessageOutput.printf("[%s] Twai driver install - no memory\r\n",
                    _providerName);
            return false;
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.printf("[%s] Twai driver install - invalid state\r\n",
                    _providerName);
            return false;
            break;
    }

    // Start TWAI driver
    twaiLastResult = twai_start();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.printf("[%s] Twai driver started\r\n",
                    _providerName);
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.printf("[%s] Twai driver start - invalid state\r\n",
                    _providerName);
            return false;
            break;
    }

    return true;
}

void BatteryCanReceiver::deinit()
{
    // Stop TWAI driver
    esp_err_t twaiLastResult = twai_stop();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.printf("[%s] Twai driver stopped\r\n",
                    _providerName);
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.printf("[%s] Twai driver stop - invalid state\r\n",
                    _providerName);
            break;
    }

    // Uninstall TWAI driver
    twaiLastResult = twai_driver_uninstall();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.printf("[%s] Twai driver uninstalled\r\n",
                    _providerName);
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.printf("[%s] Twai driver uninstall - invalid state\r\n",
                    _providerName);
            break;
    }
}

void BatteryCanReceiver::loop()
{
    // Check for messages. twai_receive is blocking when there is no data so we return if there are no frames in the buffer
    twai_status_info_t status_info;
    esp_err_t twaiLastResult = twai_get_status_info(&status_info);
    if (twaiLastResult != ESP_OK) {
        switch (twaiLastResult) {
            case ESP_ERR_INVALID_ARG:
                MessageOutput.printf("[%s] Twai driver get status - invalid arg\r\n",
                        _providerName);
                break;
            case ESP_ERR_INVALID_STATE:
                MessageOutput.printf("[%s] Twai driver get status - invalid state\r\n",
                        _providerName);
                break;
        }
        return;
    }
    if (status_info.msgs_to_rx == 0) {
        return;
    }

    // Wait for message to be received, function is blocking
    twai_message_t rx_message;
    if (twai_receive(&rx_message, pdMS_TO_TICKS(100)) != ESP_OK) {
        MessageOutput.printf("[%s] Failed to receive message",
                _providerName);
        return;
    }

    if (_verboseLogging) {
        MessageOutput.printf("[%s] Received CAN message: 0x%04X -",
                _providerName, rx_message.identifier);

        for (int i = 0; i < rx_message.data_length_code; i++) {
            MessageOutput.printf(" %02X", rx_message.data[i]);
        }

        MessageOutput.printf("\r\n");
    }

    onMessage(rx_message);
}

uint8_t BatteryCanReceiver::readUnsignedInt8(uint8_t *data)
{
    return data[0];
}

uint16_t BatteryCanReceiver::readUnsignedInt16(uint8_t *data)
{
    return (data[1] << 8) | data[0];
}

int16_t BatteryCanReceiver::readSignedInt16(uint8_t *data)
{
    return this->readUnsignedInt16(data);
}

int32_t BatteryCanReceiver::readSignedInt24(uint8_t *data)
{
    return (data[2] << 16) | (data[1] << 8) | data[0];
}

uint32_t BatteryCanReceiver::readUnsignedInt32(uint8_t *data)
{
    return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
}

float BatteryCanReceiver::scaleValue(int32_t value, float factor)
{
    return value * factor;
}

bool BatteryCanReceiver::getBit(uint8_t value, uint8_t bit)
{
    return (value & (1 << bit)) >> bit;
}

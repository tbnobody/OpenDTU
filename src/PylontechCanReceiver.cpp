// SPDX-License-Identifier: GPL-2.0-or-later
#include "PylontechCanReceiver.h"
#include "Battery.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include <driver/twai.h>
#include <ctime>

//#define PYLONTECH_DEBUG_ENABLED

PylontechCanReceiverClass PylontechCanReceiver;

void PylontechCanReceiverClass::init(int8_t rx, int8_t tx)
{
    CONFIG_T& config = Configuration.get();
    g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)tx, (gpio_num_t)rx, TWAI_MODE_NORMAL);
    if (config.Battery_Enabled) {
        enable();
    }
}

void PylontechCanReceiverClass::enable()
{
    if (_isEnabled) {
        return;
    }
    // Initialize configuration structures using macro initializers
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    twaiLastResult = twai_driver_install(&g_config, &t_config, &f_config);
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.println(F("[Pylontech] Twai driver installed"));
            break;
        case ESP_ERR_INVALID_ARG:
            MessageOutput.println(F("[Pylontech] Twai driver install - invalid arg"));
            break;
        case ESP_ERR_NO_MEM:
            MessageOutput.println(F("[Pylontech] Twai driver install - no memory"));
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.println(F("[Pylontech] Twai driver install - invalid state"));
            break;
    }

    // Start TWAI driver
    twaiLastResult = twai_start();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.println(F("[Pylontech] Twai driver started"));
            _isEnabled = true;
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.println(F("[Pylontech] Twai driver start - invalid state"));
            break;
    }
}

void PylontechCanReceiverClass::disable()
{
    if (!_isEnabled) {
        return;
    }

    // Stop TWAI driver
    twaiLastResult = twai_stop();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.println(F("[Pylontech] Twai driver stopped"));
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.println(F("[Pylontech] Twai driver stop - invalid state"));
            break;
    }

    // Uninstall TWAI driver
    twaiLastResult = twai_driver_uninstall();
    switch (twaiLastResult) {
        case ESP_OK:
            MessageOutput.println(F("[Pylontech] Twai driver uninstalled"));
            _isEnabled = false;
            break;
        case ESP_ERR_INVALID_STATE:
            MessageOutput.println(F("[Pylontech] Twai driver uninstall - invalid state"));
            break;
    }
}

void PylontechCanReceiverClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!config.Battery_Enabled) {
        return;
    }

    parseCanPackets();
    mqtt();
}

void PylontechCanReceiverClass::mqtt()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected()
            || (millis() - _lastPublish) < (config.Mqtt_PublishInterval * 1000)) {
        return;
    }    

    _lastPublish = millis();

    String topic = "battery";
    MqttSettings.publish(topic + "/settings/chargeVoltage", String(Battery.chargeVoltage));
    MqttSettings.publish(topic + "/settings/chargeCurrentLimitation", String(Battery.chargeCurrentLimitation));
    MqttSettings.publish(topic + "/settings/dischargeCurrentLimitation", String(Battery.dischargeCurrentLimitation));
    MqttSettings.publish(topic + "/stateOfCharge", String(Battery.stateOfCharge));
    MqttSettings.publish(topic + "/stateOfHealth", String(Battery.stateOfHealth));
    MqttSettings.publish(topic + "/voltage", String(Battery.voltage));
    MqttSettings.publish(topic + "/current", String(Battery.current));
    MqttSettings.publish(topic + "/temperature", String(Battery.temperature));
    MqttSettings.publish(topic + "/alarm/overCurrentDischarge", String(Battery.alarmOverCurrentDischarge));
    MqttSettings.publish(topic + "/alarm/underTemperature", String(Battery.alarmUnderTemperature));
    MqttSettings.publish(topic + "/alarm/overTemperature", String(Battery.alarmOverTemperature));
    MqttSettings.publish(topic + "/alarm/underVoltage", String(Battery.alarmUnderVoltage));
    MqttSettings.publish(topic + "/alarm/overVoltage", String(Battery.alarmOverVoltage));
    MqttSettings.publish(topic + "/alarm/bmsInternal", String(Battery.alarmBmsInternal));
    MqttSettings.publish(topic + "/alarm/overCurrentCharge", String(Battery.alarmOverCurrentCharge));
    MqttSettings.publish(topic + "/warning/highCurrentDischarge", String(Battery.warningHighCurrentDischarge));
    MqttSettings.publish(topic + "/warning/lowTemperature", String(Battery.warningLowTemperature));
    MqttSettings.publish(topic + "/warning/highTemperature", String(Battery.warningHighTemperature));
    MqttSettings.publish(topic + "/warning/lowVoltage", String(Battery.warningLowVoltage));
    MqttSettings.publish(topic + "/warning/highVoltage", String(Battery.warningHighVoltage));
    MqttSettings.publish(topic + "/warning/bmsInternal", String(Battery.warningBmsInternal));
    MqttSettings.publish(topic + "/warning/highCurrentCharge", String(Battery.warningHighCurrentCharge));
    MqttSettings.publish(topic + "/manufacturer", Battery.manufacturer);
    MqttSettings.publish(topic + "/charging/chargeEnabled", String(Battery.chargeEnabled));
    MqttSettings.publish(topic + "/charging/dischargeEnabled", String(Battery.dischargeEnabled));
    MqttSettings.publish(topic + "/charging/chargeImmediately", String(Battery.chargeImmediately));
}

void PylontechCanReceiverClass::parseCanPackets()
{
    // Check for messages. twai_receive is blocking when there is no data so we return if there are no frames in the buffer
    twai_status_info_t status_info;
    twaiLastResult = twai_get_status_info(&status_info);
    if (twaiLastResult != ESP_OK) {
        switch (twaiLastResult) {
            case ESP_ERR_INVALID_ARG:
                MessageOutput.println(F("[Pylontech] Twai driver get status - invalid arg"));
                break;
            case ESP_ERR_INVALID_STATE:
                MessageOutput.println(F("[Pylontech] Twai driver get status - invalid state"));
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
        MessageOutput.println(F("[Pylontech] Failed to receive message"));
        return;
    }

    switch (rx_message.identifier) {
        case 0x351: {
            Battery.chargeVoltage = this->scaleValue(this->readUnsignedInt16(rx_message.data), 0.1);
            Battery.chargeCurrentLimitation = this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1);
            Battery.dischargeCurrentLimitation = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

#ifdef PYLONTECH_DEBUG_ENABLED
           MessageOutput.printf("[Pylontech] chargeVoltage: %f chargeCurrentLimitation: %f dischargeCurrentLimitation: %f\n",
                Battery.chargeVoltage, Battery.chargeCurrentLimitation, Battery.dischargeCurrentLimitation);
#endif
            break;
        }

        case 0x355: {
            Battery.stateOfCharge = this->readUnsignedInt16(rx_message.data);
            Battery.stateOfChargeLastUpdate = millis();
            Battery.stateOfHealth = this->readUnsignedInt16(rx_message.data + 2);
            Battery.lastUpdate = millis();

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] soc: %d soh: %d\n",
                Battery.stateOfCharge, Battery.stateOfHealth);
#endif
            break;
        }

        case 0x356: {
            Battery.voltage = this->scaleValue(this->readSignedInt16(rx_message.data), 0.01);
            Battery.current = this->scaleValue(this->readSignedInt16(rx_message.data + 2), 0.1);
            Battery.temperature = this->scaleValue(this->readSignedInt16(rx_message.data + 4), 0.1);

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] voltage: %f current: %f temperature: %f\n",
                Battery.voltage, Battery.current, Battery.temperature);
#endif
            break;
        }

        case 0x359: {
            uint16_t alarmBits = rx_message.data[0];
            Battery.alarmOverCurrentDischarge = this->getBit(alarmBits, 7);
            Battery.alarmUnderTemperature = this->getBit(alarmBits, 4);
            Battery.alarmOverTemperature = this->getBit(alarmBits, 3);
            Battery.alarmUnderVoltage = this->getBit(alarmBits, 2);
            Battery.alarmOverVoltage= this->getBit(alarmBits, 1);

            alarmBits = rx_message.data[1];
            Battery.alarmBmsInternal= this->getBit(alarmBits, 3);
            Battery.alarmOverCurrentCharge = this->getBit(alarmBits, 0);

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] Alarms: %d %d %d %d %d %d %d\n",
                Battery.alarmOverCurrentDischarge,
                Battery.alarmUnderTemperature,
                Battery.alarmOverTemperature,
                Battery.alarmUnderVoltage,
                Battery.alarmOverVoltage,
                Battery.alarmBmsInternal,
                Battery.alarmOverCurrentCharge);
#endif

            uint16_t warningBits = rx_message.data[2];
            Battery.warningHighCurrentDischarge = this->getBit(warningBits, 7);
            Battery.warningLowTemperature = this->getBit(warningBits, 4);
            Battery.warningHighTemperature = this->getBit(warningBits, 3);
            Battery.warningLowVoltage = this->getBit(warningBits, 2);
            Battery.warningHighVoltage = this->getBit(warningBits, 1);

            warningBits = rx_message.data[3];
            Battery.warningBmsInternal= this->getBit(warningBits, 3);
            Battery.warningHighCurrentCharge = this->getBit(warningBits, 0);

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] Warnings: %d %d %d %d %d %d %d\n",
                Battery.warningHighCurrentDischarge,
                Battery.warningLowTemperature,
                Battery.warningHighTemperature,
                Battery.warningLowVoltage,
                Battery.warningHighVoltage,
                Battery.warningBmsInternal,
                Battery.warningHighCurrentCharge);
#endif
            break;
        }

        case 0x35E: {
            String manufacturer = String(rx_message.data, rx_message.data_length_code);
            //CAN.readString();

            if (manufacturer == "") {
                break;
            }

            strlcpy(Battery.manufacturer, manufacturer.c_str(), sizeof(Battery.manufacturer));

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] Manufacturer: %s\n", manufacturer.c_str());
#endif   
            break;
        }

        case 0x35C: {
            uint16_t chargeStatusBits = rx_message.data[0];
            Battery.chargeEnabled = this->getBit(chargeStatusBits, 7);
            Battery.dischargeEnabled = this->getBit(chargeStatusBits, 6);
            Battery.chargeImmediately = this->getBit(chargeStatusBits, 5);

#ifdef PYLONTECH_DEBUG_ENABLED
            MessageOutput.printf("[Pylontech] chargeStatusBits: %d %d %d\n",
                Battery.chargeEnabled,
                Battery.dischargeEnabled,
                Battery.chargeImmediately);
#endif

            break;
        }
    }
}

uint16_t PylontechCanReceiverClass::readUnsignedInt16(uint8_t *data)
{
    uint8_t bytes[2];
    bytes[0] = *data;
    bytes[1] = *(data + 1);
    return (bytes[1] << 8) + bytes[0];
}

int16_t PylontechCanReceiverClass::readSignedInt16(uint8_t *data)
{
    return this->readUnsignedInt16(data);
}

float PylontechCanReceiverClass::scaleValue(int16_t value, float factor)
{
    return value * factor;
}

bool PylontechCanReceiverClass::getBit(uint8_t value, uint8_t bit)
{
    return (value & (1 << bit)) >> bit;
}

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <driver/twai.h>
#include <Arduino.h>
#include <memory>

#ifndef PYLONTECH_PIN_RX
#define PYLONTECH_PIN_RX 27
#endif

#ifndef PYLONTECH_PIN_TX
#define PYLONTECH_PIN_TX 26
#endif

class PylontechCanReceiverClass {
public:
    void init(int8_t rx, int8_t tx);
    void enable();
    void loop();
    void parseCanPackets();
    void mqtt();

private:
    uint16_t readUnsignedInt16(uint8_t *data);
    int16_t readSignedInt16(uint8_t *data);
    void readString(char* str, uint8_t numBytes);
    void readBooleanBits8(bool* b, uint8_t numBits);
    float scaleValue(int16_t value, float factor);
    bool getBit(uint8_t value, uint8_t bit);

    uint32_t _lastPublish;
    twai_general_config_t g_config;

};

extern PylontechCanReceiverClass PylontechCanReceiver;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <Arduino.h>
#include <Hoymiles.h>
#include <memory>
#include "SDM.h"

#ifndef SDM_RX_PIN
#define SDM_RX_PIN 13
#endif

#ifndef SDM_TX_PIN
#define SDM_TX_PIN 32
#endif

class PowerMeterClass {
public:
    enum SOURCE {
        SOURCE_MQTT = 0,
        SOURCE_SDM1PH = 1,
        SOURCE_SDM3PH = 2,
        SOURCE_HTTP = 3,
    };
    void init();
    void mqtt();
    void loop();
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
    float getPowerTotal();
    uint32_t getLastPowerMeterUpdate();

private:
    uint32_t _interval;
    uint32_t _lastPowerMeterCheck;
    // Used in Power limiter for safety check
    uint32_t _lastPowerMeterUpdate;

    float _powerMeter1Power = 0.0;
    float _powerMeter2Power = 0.0;
    float _powerMeter3Power = 0.0;
    float _powerMeterTotalPower = 0.0;
    float _powerMeter1Voltage = 0.0;
    float _powerMeter2Voltage = 0.0;
    float _powerMeter3Voltage = 0.0;
    float _PowerMeterImport = 0.0;
    float _PowerMeterExport = 0.0;

    bool mqttInitDone = false;
};

extern PowerMeterClass PowerMeter;

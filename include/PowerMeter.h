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
    void init();
    void mqtt();
    void loop();
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);
    float getPowerTotal();

private:
    uint32_t _interval;
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
    char PowerMeter_MqttTopicPowerMeter1old[MQTT_MAX_TOPIC_STRLEN + 1];
    char PowerMeter_MqttTopicPowerMeter2old[MQTT_MAX_TOPIC_STRLEN + 1];
    char PowerMeter_MqttTopicPowerMeter3old[MQTT_MAX_TOPIC_STRLEN + 1];

};

extern PowerMeterClass PowerMeter;

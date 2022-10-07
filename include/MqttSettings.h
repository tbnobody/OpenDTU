// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "NetworkSettings.h"
#include <Arduino.h>
#include <Ticker.h>
#include <espMqttClient.h>
#include <memory>

class MqttSettingsClass {
public:
    MqttSettingsClass();
    void init();
    void performReconnect();
    bool getConnected();
    void publish(String subtopic, String payload);
    void publishHass(String subtopic, String payload);

    String getPrefix();

private:
    void NetworkEvent(network_event event);

    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
    void onMqttConnect(bool sessionPresent);
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    void performConnect();
    void performDisconnect();

    void createMqttClientObject();

    MqttClient* mqttClient = nullptr;
    String clientId;
    String willTopic;
    Ticker mqttReconnectTimer;
};

extern MqttSettingsClass MqttSettings;
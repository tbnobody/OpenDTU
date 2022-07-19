// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "NetworkSettings.h"
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
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

    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttConnect(bool sessionPresent);

    void performConnect();
    void performDisconnect();

    AsyncMqttClient mqttClient;
    String clientId;
    String willTopic;
    Ticker mqttReconnectTimer;
};

extern MqttSettingsClass MqttSettings;
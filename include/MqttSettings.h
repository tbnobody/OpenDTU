#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <WiFi.h>
#include <memory>

class MqttSettingsClass {
public:
    MqttSettingsClass();
    void init();
    void performReconnect();
    bool getConnected();
    void publish(String subtopic, String payload);

private:
    void WiFiEvent(WiFiEvent_t event);

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
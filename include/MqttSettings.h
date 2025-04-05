// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "NetworkSettings.h"
#include <MqttSubscribeParser.h>
#include <Ticker.h>
#include <espMqttClient.h>
#include <mutex>

typedef std::function<void(uint16_t packetId)> MqttOnPublishCallback;
typedef uint16_t PacketId;

class MqttSettingsClass {
public:
    MqttSettingsClass();
    void init();
    void performReconnect();
    bool getConnected();
    PacketId publish(const String& subtopic, const String& payload, const uint8_t qos = 0);
    PacketId publishGeneric(const String& topic, const String& payload, const bool retain, const uint8_t qos = 0);

    void subscribe(const String& topic, const uint8_t qos, const espMqttClientTypes::OnMessageCallback& cb);
    void unsubscribe(const String& topic);

    String getPrefix() const;
    String getClientId() const;

    void addOnPublishCallback(MqttOnPublishCallback callback);

private:
    void NetworkEvent(network_event event);

    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
    void onMqttConnect(const bool sessionPresent);
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total);
    void onMqttPublish(PacketId packetId);

    void performConnect();
    void performDisconnect();

    void createMqttClientObject();

    MqttClient* _mqttClient = nullptr;
    Ticker _mqttReconnectTimer;
    MqttSubscribeParser _mqttSubscribeParser;
    std::mutex _clientLock;
    MqttOnPublishCallback _onPacketCallback;
    
};

extern MqttSettingsClass MqttSettings;

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttSettings.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include <AsyncMqttClient.h>
#include <Ticker.h>

MqttSettingsClass::MqttSettingsClass()
    : mqttClient()
{
}

void MqttSettingsClass::NetworkEvent(network_event event)
{
    switch (event) {
    case network_event::NETWORK_GOT_IP:
        Serial.println(F("Network connected"));
        performConnect();
        break;
    case network_event::NETWORK_DISCONNECTED:
        Serial.println(F("Network lost connection"));
        mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        break;
    }
}

void MqttSettingsClass::onMqttConnect(bool sessionPresent)
{
    Serial.println(F("Connected to MQTT."));
    CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Online);
}

void MqttSettingsClass::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println(F("Disconnected from MQTT."));

    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

void MqttSettingsClass::performConnect()
{
    if (NetworkSettings.isConnected() && Configuration.get().Mqtt_Enabled) {
        Serial.println(F("Connecting to MQTT..."));
        CONFIG_T& config = Configuration.get();
        mqttClient.setServer(config.Mqtt_Hostname, config.Mqtt_Port);
        mqttClient.setCredentials(config.Mqtt_Username, config.Mqtt_Password);

        willTopic = getPrefix() + config.Mqtt_LwtTopic;
        mqttClient.setWill(willTopic.c_str(), 2, config.Mqtt_Retain, config.Mqtt_LwtValue_Offline);

        clientId = NetworkSettings.getApName();
        mqttClient.setClientId(clientId.c_str());

        mqttClient.connect();
    }
}

void MqttSettingsClass::performDisconnect()
{
    CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Offline);
    mqttClient.disconnect();
}

void MqttSettingsClass::performReconnect()
{
    performDisconnect();

    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

bool MqttSettingsClass::getConnected()
{
    return mqttClient.connected();
}

String MqttSettingsClass::getPrefix()
{
    return Configuration.get().Mqtt_Topic;
}

void MqttSettingsClass::publish(String subtopic, String payload)
{
    String topic = getPrefix();
    topic += subtopic;
    mqttClient.publish(topic.c_str(), 0, Configuration.get().Mqtt_Retain, payload.c_str());
}

void MqttSettingsClass::publishHass(String subtopic, String payload)
{
    String topic = Configuration.get().Mqtt_Hass_Topic;
    topic += subtopic;
    mqttClient.publish(topic.c_str(), 0, Configuration.get().Mqtt_Hass_Retain, payload.c_str());
}

void MqttSettingsClass::init()
{
    using namespace std::placeholders;
    NetworkSettings.onEvent(std::bind(&MqttSettingsClass::NetworkEvent, this, _1));

    mqttClient.onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
    mqttClient.onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
}

MqttSettingsClass MqttSettings;
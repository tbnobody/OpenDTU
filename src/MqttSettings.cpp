// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttSettings.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include <MqttClientSetup.h>
#include <Ticker.h>
#include <espMqttClient.h>

MqttSettingsClass::MqttSettingsClass()
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

void MqttSettingsClass::onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    Serial.println(F("Disconnected from MQTT."));

    Serial.print(F("Disconnect reason:"));
    switch (reason) {
    case espMqttClientTypes::DisconnectReason::TCP_DISCONNECTED:
        Serial.println(F("TCP_DISCONNECTED"));
        break;
    case espMqttClientTypes::DisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
        Serial.println(F("MQTT_UNACCEPTABLE_PROTOCOL_VERSION"));
        break;
    case espMqttClientTypes::DisconnectReason::MQTT_IDENTIFIER_REJECTED:
        Serial.println(F("MQTT_IDENTIFIER_REJECTED"));
        break;
    case espMqttClientTypes::DisconnectReason::MQTT_SERVER_UNAVAILABLE:
        Serial.println(F("MQTT_SERVER_UNAVAILABLE"));
        break;
    case espMqttClientTypes::DisconnectReason::MQTT_MALFORMED_CREDENTIALS:
        Serial.println(F("MQTT_MALFORMED_CREDENTIALS"));
        break;
    case espMqttClientTypes::DisconnectReason::MQTT_NOT_AUTHORIZED:
        Serial.println(F("MQTT_NOT_AUTHORIZED"));
        break;
    default:
        Serial.println(F("Unknown"));
    }
    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

void MqttSettingsClass::performConnect()
{
    if (NetworkSettings.isConnected() && Configuration.get().Mqtt_Enabled) {
        using namespace std::placeholders;
        Serial.println(F("Connecting to MQTT..."));
        CONFIG_T& config = Configuration.get();
        willTopic = getPrefix() + config.Mqtt_LwtTopic;
        clientId = NetworkSettings.getApName();
        if (config.Mqtt_Tls) {
            static_cast<espMqttClientSecure*>(mqttClient)->setCACert(config.Mqtt_RootCaCert);
            static_cast<espMqttClientSecure*>(mqttClient)->setServer(config.Mqtt_Hostname, config.Mqtt_Port);
            static_cast<espMqttClientSecure*>(mqttClient)->setCredentials(config.Mqtt_Username, config.Mqtt_Password);
            static_cast<espMqttClientSecure*>(mqttClient)->setWill(willTopic.c_str(), 2, config.Mqtt_Retain, config.Mqtt_LwtValue_Offline);
            static_cast<espMqttClientSecure*>(mqttClient)->setClientId(clientId.c_str());
            static_cast<espMqttClientSecure*>(mqttClient)->onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
            static_cast<espMqttClientSecure*>(mqttClient)->onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
        } else {
            static_cast<espMqttClient*>(mqttClient)->setServer(config.Mqtt_Hostname, config.Mqtt_Port);
            static_cast<espMqttClient*>(mqttClient)->setCredentials(config.Mqtt_Username, config.Mqtt_Password);
            static_cast<espMqttClient*>(mqttClient)->setWill(willTopic.c_str(), 2, config.Mqtt_Retain, config.Mqtt_LwtValue_Offline);
            static_cast<espMqttClient*>(mqttClient)->setClientId(clientId.c_str());
            static_cast<espMqttClient*>(mqttClient)->onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
            static_cast<espMqttClient*>(mqttClient)->onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
        }
        mqttClient->connect();
    }
}

void MqttSettingsClass::performDisconnect()
{
    CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Offline);
    mqttClient->disconnect();
}

void MqttSettingsClass::performReconnect()
{
    performDisconnect();

    createMqttClientObject();

    mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

bool MqttSettingsClass::getConnected()
{
    return mqttClient->connected();
}

String MqttSettingsClass::getPrefix()
{
    return Configuration.get().Mqtt_Topic;
}

void MqttSettingsClass::publish(String subtopic, String payload)
{
    String topic = getPrefix();
    topic += subtopic;
    mqttClient->publish(topic.c_str(), 0, Configuration.get().Mqtt_Retain, payload.c_str());
}

void MqttSettingsClass::publishHass(String subtopic, String payload)
{
    String topic = Configuration.get().Mqtt_Hass_Topic;
    topic += subtopic;
    mqttClient->publish(topic.c_str(), 0, Configuration.get().Mqtt_Hass_Retain, payload.c_str());
}

void MqttSettingsClass::init()
{
    using namespace std::placeholders;
    NetworkSettings.onEvent(std::bind(&MqttSettingsClass::NetworkEvent, this, _1));

    createMqttClientObject();
}

void MqttSettingsClass::createMqttClientObject()
{
    if (mqttClient != nullptr)
        delete mqttClient;
    CONFIG_T& config = Configuration.get();
    if (config.Mqtt_Tls) {
        mqttClient = static_cast<MqttClient*>(new espMqttClientSecure);
    } else {
        mqttClient = static_cast<MqttClient*>(new espMqttClient);
    }
}

MqttSettingsClass MqttSettings;
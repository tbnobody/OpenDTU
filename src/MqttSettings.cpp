// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "MqttSettings.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include <frozen/map.h>
#include <frozen/string.h>

MqttSettingsClass::MqttSettingsClass()
{
}

void MqttSettingsClass::NetworkEvent(network_event event)
{
    switch (event) {
    case network_event::NETWORK_GOT_IP:
        MessageOutput.printf("Network connected\r\n");
        performConnect();
        break;
    case network_event::NETWORK_DISCONNECTED:
        MessageOutput.printf("Network lost connection\r\n");
        _mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        break;
    default:
        break;
    }
}

void MqttSettingsClass::onMqttConnect(const bool sessionPresent)
{
    MessageOutput.printf("Connected to MQTT.\r\n");
    const CONFIG_T& config = Configuration.get();
    publish(config.Mqtt.Lwt.Topic, config.Mqtt.Lwt.Value_Online);

    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient != nullptr) {
        for (const auto& cb : _mqttSubscribeParser.get_callbacks()) {
            _mqttClient->subscribe(cb.topic.c_str(), cb.qos);
        }
    }
}

void MqttSettingsClass::subscribe(const String& topic, const uint8_t qos, const OnMessageCallback& cb)
{
    _mqttSubscribeParser.register_callback(topic.c_str(), qos, cb);
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient != nullptr) {
        _mqttClient->subscribe(topic.c_str(), qos);
    }
}

void MqttSettingsClass::unsubscribe(const String& topic)
{
    _mqttSubscribeParser.unregister_callback(topic.c_str());
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient != nullptr) {
        _mqttClient->unsubscribe(topic.c_str());
    }
}

void MqttSettingsClass::onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    static constexpr frozen::map<espMqttClientTypes::DisconnectReason, frozen::string, 8> reasons = {
        { espMqttClientTypes::DisconnectReason::USER_OK, "USER_OK" },
        { espMqttClientTypes::DisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION, "MQTT_UNACCEPTABLE_PROTOCOL_VERSION" },
        { espMqttClientTypes::DisconnectReason::MQTT_IDENTIFIER_REJECTED, "MQTT_IDENTIFIER_REJECTED" },
        { espMqttClientTypes::DisconnectReason::MQTT_SERVER_UNAVAILABLE, "MQTT_SERVER_UNAVAILABLE" },
        { espMqttClientTypes::DisconnectReason::MQTT_MALFORMED_CREDENTIALS, "MQTT_MALFORMED_CREDENTIALS" },
        { espMqttClientTypes::DisconnectReason::MQTT_NOT_AUTHORIZED, "MQTT_NOT_AUTHORIZED" },
        { espMqttClientTypes::DisconnectReason::TLS_BAD_FINGERPRINT, "TLS_BAD_FINGERPRINT" },
        { espMqttClientTypes::DisconnectReason::TCP_DISCONNECTED, "TCP_DISCONNECTED" },
    };

    auto it = reasons.find(reason);
    const char* reasonStr = (it != reasons.end()) ? it->second.data() : "Unknown";

    MessageOutput.printf("Disconnected from MQTT. Reason: %s\r\n", reasonStr);

    _mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

void MqttSettingsClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total)
{
    MessageOutput.printf("Received MQTT message on topic: %s\r\n", topic);

    _mqttSubscribeParser.handle_message(properties, topic, payload, len);
}

void MqttSettingsClass::performConnect()
{
    if (NetworkSettings.isConnected() && Configuration.get().Mqtt.Enabled) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;

        std::lock_guard<std::mutex> lock(_clientLock);
        if (_mqttClient == nullptr) {
            return;
        }

        MessageOutput.printf("Connecting to MQTT...\r\n");
        const CONFIG_T& config = Configuration.get();
        const String willTopic = getPrefix() + config.Mqtt.Lwt.Topic;
        String clientId = getClientId();
        if (config.Mqtt.Tls.Enabled) {
            static_cast<espMqttClientSecure*>(_mqttClient)->setCACert(config.Mqtt.Tls.RootCaCert);
            static_cast<espMqttClientSecure*>(_mqttClient)->setServer(config.Mqtt.Hostname, config.Mqtt.Port);
            if (config.Mqtt.Tls.CertLogin) {
                static_cast<espMqttClientSecure*>(_mqttClient)->setCertificate(config.Mqtt.Tls.ClientCert);
                static_cast<espMqttClientSecure*>(_mqttClient)->setPrivateKey(config.Mqtt.Tls.ClientKey);
            } else {
                static_cast<espMqttClientSecure*>(_mqttClient)->setCredentials(config.Mqtt.Username, config.Mqtt.Password);
            }
            static_cast<espMqttClientSecure*>(_mqttClient)->setWill(willTopic.c_str(), config.Mqtt.Lwt.Qos, config.Mqtt.Retain, config.Mqtt.Lwt.Value_Offline);
            static_cast<espMqttClientSecure*>(_mqttClient)->setClientId(clientId.c_str());
            static_cast<espMqttClientSecure*>(_mqttClient)->setCleanSession(config.Mqtt.CleanSession);
            static_cast<espMqttClientSecure*>(_mqttClient)->onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
            static_cast<espMqttClientSecure*>(_mqttClient)->onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
            static_cast<espMqttClientSecure*>(_mqttClient)->onMessage(std::bind(&MqttSettingsClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
        } else {
            static_cast<espMqttClient*>(_mqttClient)->setServer(config.Mqtt.Hostname, config.Mqtt.Port);
            static_cast<espMqttClient*>(_mqttClient)->setCredentials(config.Mqtt.Username, config.Mqtt.Password);
            static_cast<espMqttClient*>(_mqttClient)->setWill(willTopic.c_str(), config.Mqtt.Lwt.Qos, config.Mqtt.Retain, config.Mqtt.Lwt.Value_Offline);
            static_cast<espMqttClient*>(_mqttClient)->setClientId(clientId.c_str());
            static_cast<espMqttClient*>(_mqttClient)->setCleanSession(config.Mqtt.CleanSession);
            static_cast<espMqttClient*>(_mqttClient)->onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
            static_cast<espMqttClient*>(_mqttClient)->onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
            static_cast<espMqttClient*>(_mqttClient)->onMessage(std::bind(&MqttSettingsClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
        }
        _mqttClient->connect();
    }
}

void MqttSettingsClass::performDisconnect()
{
    const CONFIG_T& config = Configuration.get();
    publish(config.Mqtt.Lwt.Topic, config.Mqtt.Lwt.Value_Offline);
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient == nullptr) {
        return;
    }
    _mqttClient->disconnect();
}

void MqttSettingsClass::performReconnect()
{
    performDisconnect();

    createMqttClientObject();

    _mqttReconnectTimer.once(
        2, +[](MqttSettingsClass* instance) { instance->performConnect(); }, this);
}

bool MqttSettingsClass::getConnected()
{
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient == nullptr) {
        return false;
    }
    return _mqttClient->connected();
}

String MqttSettingsClass::getPrefix() const
{
    return Configuration.get().Mqtt.Topic;
}

String MqttSettingsClass::getClientId() const
{
    String clientId = Configuration.get().Mqtt.ClientId;
    if (clientId == "") {
        clientId = NetworkSettings.getApName();
    }
    return clientId;
}

void MqttSettingsClass::publish(const String& subtopic, const String& payload)
{
    String topic = getPrefix();
    topic += subtopic;

    String value = payload;
    value.trim();

    publishGeneric(topic, value, Configuration.get().Mqtt.Retain, 0);
}

void MqttSettingsClass::publishGeneric(const String& topic, const String& payload, const bool retain, const uint8_t qos)
{
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient == nullptr) {
        return;
    }
    _mqttClient->publish(topic.c_str(), qos, retain, payload.c_str());
}

void MqttSettingsClass::init()
{
    using std::placeholders::_1;
    NetworkSettings.onEvent(std::bind(&MqttSettingsClass::NetworkEvent, this, _1));

    createMqttClientObject();
}

void MqttSettingsClass::createMqttClientObject()
{
    std::lock_guard<std::mutex> lock(_clientLock);
    if (_mqttClient != nullptr) {
        delete _mqttClient;
        _mqttClient = nullptr;
    }
    const CONFIG_T& config = Configuration.get();
    if (config.Mqtt.Tls.Enabled) {
        _mqttClient = static_cast<MqttClient*>(new espMqttClientSecure);
    } else {
        _mqttClient = static_cast<MqttClient*>(new espMqttClient);
    }
}

MqttSettingsClass MqttSettings;

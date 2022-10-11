// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttSettings.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>
#include <MqttClientSetup.h>
#include <Ticker.h>
#include <espMqttClient.h>

#define TOPIC_SUB_LIMIT_PERSISTENT_RELATIVE "limit_persistent_relative"
#define TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE "limit_persistent_absolute"
#define TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE "limit_nonpersistent_relative"
#define TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE "limit_nonpersistent_absolute"
#define TOPIC_SUB_POWER "power"
#define TOPIC_SUB_RESTART "restart"

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
    default:
        break;
    }
}

void MqttSettingsClass::onMqttConnect(bool sessionPresent)
{
    Serial.println(F("Connected to MQTT."));
    const CONFIG_T& config = Configuration.get();
    publish(config.Mqtt_LwtTopic, config.Mqtt_LwtValue_Online);

    String topic = getPrefix();
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_PERSISTENT_RELATIVE).c_str(), 0);
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE).c_str(), 0);
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE).c_str(), 0);
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE).c_str(), 0);
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_POWER).c_str(), 0);
    mqttClient->subscribe(String(topic + "+/cmd/" + TOPIC_SUB_RESTART).c_str(), 0);
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

void MqttSettingsClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    const CONFIG_T& config = Configuration.get();

    Serial.print(F("Received MQTT message on topic: "));
    Serial.println(topic);

    char token_topic[MQTT_MAX_TOPIC_STRLEN + 40]; // respect all subtopics
    strncpy(token_topic, topic, MQTT_MAX_TOPIC_STRLEN + 40); // convert const char* to char*

    char* serial_str;
    char* subtopic;
    char* setting;
    char* rest = &token_topic[strlen(config.Mqtt_Topic)];

    serial_str = strtok_r(rest, "/", &rest);
    subtopic = strtok_r(rest, "/", &rest);
    setting = strtok_r(rest, "/", &rest);

    if (serial_str == NULL || subtopic == NULL || setting == NULL) {
        return;
    }

    uint64_t serial;
    serial = strtoull(serial_str, 0, 16);

    auto inv = Hoymiles.getInverterBySerial(serial);

    if (inv == nullptr) {
        Serial.println(F("Inverter not found"));
        return;
    }

    // check if subtopic is unequal cmd
    if (strcmp(subtopic, "cmd")) {
        return;
    }

    char* strlimit = new char[len + 1];
    memcpy(strlimit, payload, len);
    strlimit[len] = '\0';
    uint32_t payload_val = strtol(strlimit, NULL, 10);
    delete[] strlimit;

    if (!strcmp(setting, TOPIC_SUB_LIMIT_PERSISTENT_RELATIVE)) {
        // Set inverter limit relative persistent
        Serial.printf("Limit Persistent: %d %%\n", payload_val);
        inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::RelativPersistent);

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE)) {
        // Set inverter limit absolute persistent
        Serial.printf("Limit Persistent: %d W\n", payload_val);
        inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::AbsolutPersistent);

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE)) {
        // Set inverter limit relative non persistent
        Serial.printf("Limit Non-Persistent: %d %%\n", payload_val);
        if (!properties.retain) {
            inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::RelativNonPersistent);
        } else {
            Serial.println("Ignored because retained");
        }

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE)) {
        // Set inverter limit absolute non persistent
        Serial.printf("Limit Non-Persistent: %d W\n", payload_val);
        if (!properties.retain) {
            inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::AbsolutNonPersistent);
        } else {
            Serial.println("Ignored because retained");
        }

    } else if (!strcmp(setting, TOPIC_SUB_POWER)) {
        // Turn inverter on or off
        Serial.printf("Set inverter power to: %d\n", payload_val);
        inv->sendPowerControlRequest(Hoymiles.getRadio(), payload_val > 0);

    } else if (!strcmp(setting, TOPIC_SUB_RESTART)) {
        // Restart inverter
        Serial.printf("Restart inverter\n");
        if (!properties.retain && payload_val == 1) {
            inv->sendRestartControlRequest(Hoymiles.getRadio());
        } else {
            Serial.println("Ignored because retained");
        }
    }
}

void MqttSettingsClass::performConnect()
{
    if (NetworkSettings.isConnected() && Configuration.get().Mqtt_Enabled) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::placeholders::_4;
        using std::placeholders::_5;
        using std::placeholders::_6;
        Serial.println(F("Connecting to MQTT..."));
        const CONFIG_T& config = Configuration.get();
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
            static_cast<espMqttClientSecure*>(mqttClient)->onMessage(std::bind(&MqttSettingsClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
        } else {
            static_cast<espMqttClient*>(mqttClient)->setServer(config.Mqtt_Hostname, config.Mqtt_Port);
            static_cast<espMqttClient*>(mqttClient)->setCredentials(config.Mqtt_Username, config.Mqtt_Password);
            static_cast<espMqttClient*>(mqttClient)->setWill(willTopic.c_str(), 2, config.Mqtt_Retain, config.Mqtt_LwtValue_Offline);
            static_cast<espMqttClient*>(mqttClient)->setClientId(clientId.c_str());
            static_cast<espMqttClient*>(mqttClient)->onConnect(std::bind(&MqttSettingsClass::onMqttConnect, this, _1));
            static_cast<espMqttClient*>(mqttClient)->onDisconnect(std::bind(&MqttSettingsClass::onMqttDisconnect, this, _1));
            static_cast<espMqttClient*>(mqttClient)->onMessage(std::bind(&MqttSettingsClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
        }
        mqttClient->connect();
    }
}

void MqttSettingsClass::performDisconnect()
{
    const CONFIG_T& config = Configuration.get();
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
    using std::placeholders::_1;
    NetworkSettings.onEvent(std::bind(&MqttSettingsClass::NetworkEvent, this, _1));

    createMqttClientObject();
}

void MqttSettingsClass::createMqttClientObject()
{
    if (mqttClient != nullptr)
        delete mqttClient;
    const CONFIG_T& config = Configuration.get();
    if (config.Mqtt_Tls) {
        mqttClient = static_cast<MqttClient*>(new espMqttClientSecure);
    } else {
        mqttClient = static_cast<MqttClient*>(new espMqttClient);
    }
}

MqttSettingsClass MqttSettings;
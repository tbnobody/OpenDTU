// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHandleInverter.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include <ctime>

#define TOPIC_SUB_LIMIT_PERSISTENT_RELATIVE "limit_persistent_relative"
#define TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE "limit_persistent_absolute"
#define TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE "limit_nonpersistent_relative"
#define TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE "limit_nonpersistent_absolute"
#define TOPIC_SUB_POWER "power"
#define TOPIC_SUB_RESTART "restart"

MqttHandleInverterClass MqttHandleInverter;

void MqttHandleInverterClass::init()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix();
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_PERSISTENT_RELATIVE).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_POWER).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    MqttSettings.subscribe(String(topic + "+/cmd/" + TOPIC_SUB_RESTART).c_str(), 0, std::bind(&MqttHandleInverterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
}

void MqttHandleInverterClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.getRadio()->isIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            String subtopic = inv->serialString();

            // Name
            MqttSettings.publish(subtopic + "/name", inv->name());

            if (inv->DevInfo()->getLastUpdate() > 0) {
                // Bootloader Version
                MqttSettings.publish(subtopic + "/device/bootloaderversion", String(inv->DevInfo()->getFwBootloaderVersion()));

                // Firmware Version
                MqttSettings.publish(subtopic + "/device/fwbuildversion", String(inv->DevInfo()->getFwBuildVersion()));

                // Firmware Build DateTime
                char timebuffer[32];
                const time_t t = inv->DevInfo()->getFwBuildDateTime();
                std::strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
                MqttSettings.publish(subtopic + "/device/fwbuilddatetime", String(timebuffer));

                // Hardware part number
                MqttSettings.publish(subtopic + "/device/hwpartnumber", String(inv->DevInfo()->getHwPartNumber()));

                // Hardware version
                MqttSettings.publish(subtopic + "/device/hwversion", inv->DevInfo()->getHwVersion());
            }

            if (inv->SystemConfigPara()->getLastUpdate() > 0) {
                // Limit
                MqttSettings.publish(subtopic + "/status/limit_relative", String(inv->SystemConfigPara()->getLimitPercent()));

                uint16_t maxpower = inv->DevInfo()->getMaxPower();
                if (maxpower > 0) {
                    MqttSettings.publish(subtopic + "/status/limit_absolute", String(inv->SystemConfigPara()->getLimitPercent() * maxpower / 100));
                }
            }

            MqttSettings.publish(subtopic + "/status/reachable", String(inv->isReachable()));
            MqttSettings.publish(subtopic + "/status/producing", String(inv->isProducing()));

            if (inv->Statistics()->getLastUpdate() > 0) {
                MqttSettings.publish(subtopic + "/status/last_update", String(std::time(0) - (millis() - inv->Statistics()->getLastUpdate()) / 1000));
            } else {
                MqttSettings.publish(subtopic + "/status/last_update", String(0));
            }

            uint32_t lastUpdate = inv->Statistics()->getLastUpdate();
            if (lastUpdate > 0 && lastUpdate != _lastPublishStats[i]) {
                _lastPublishStats[i] = lastUpdate;

                // Loop all channels
                for (auto& t : inv->Statistics()->getChannelTypes()) {
                    for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                        if (t == TYPE_DC) {
                            INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
                            if (inv_cfg != nullptr) {
                                // TODO(tbnobody)
                                MqttSettings.publish(inv->serialString() + "/" + String(static_cast<uint8_t>(c) + 1) + "/name", inv_cfg->channel[c].Name);
                            }
                        }
                        for (uint8_t f = 0; f < sizeof(_publishFields) / sizeof(FieldId_t); f++) {
                            publishField(inv, t, c, _publishFields[f]);
                        }
                    }
                }
            }

            yield();
        }

        _lastPublish = millis();
    }
}

void MqttHandleInverterClass::publishField(std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    String topic = getTopic(inv, type, channel, fieldId);
    if (topic == "") {
        return;
    }

    MqttSettings.publish(topic, String(
        inv->Statistics()->getChannelFieldValue(type, channel, fieldId),
        static_cast<unsigned int>(inv->Statistics()->getChannelFieldDigits(type, channel, fieldId))));
}

String MqttHandleInverterClass::getTopic(std::shared_ptr<InverterAbstract> inv, ChannelType_t type, ChannelNum_t channel, FieldId_t fieldId)
{
    if (!inv->Statistics()->hasChannelFieldValue(type, channel, fieldId)) {
        return String("");
    }

    String chanName;
    if (type == TYPE_AC && fieldId == FLD_PDC) {
        chanName = "powerdc";
    } else {
        chanName = inv->Statistics()->getChannelFieldName(type, channel, fieldId);
        chanName.toLowerCase();
    }

    String chanNum;
    if (type == TYPE_DC) {
        // TODO(tbnobody)
        chanNum = static_cast<uint8_t>(channel) + 1;
    } else {
        chanNum = channel;
    }

    return inv->serialString() + "/" + chanNum + "/" + chanName;
}

void MqttHandleInverterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total)
{
    const CONFIG_T& config = Configuration.get();

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
        MessageOutput.println(F("Inverter not found"));
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
        MessageOutput.printf("Limit Persistent: %d %%\r\n", payload_val);
        inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::RelativPersistent);

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_PERSISTENT_ABSOLUTE)) {
        // Set inverter limit absolute persistent
        MessageOutput.printf("Limit Persistent: %d W\r\n", payload_val);
        inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::AbsolutPersistent);

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_NONPERSISTENT_RELATIVE)) {
        // Set inverter limit relative non persistent
        MessageOutput.printf("Limit Non-Persistent: %d %%\r\n", payload_val);
        if (!properties.retain) {
            inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::RelativNonPersistent);
        } else {
            MessageOutput.println("Ignored because retained");
        }

    } else if (!strcmp(setting, TOPIC_SUB_LIMIT_NONPERSISTENT_ABSOLUTE)) {
        // Set inverter limit absolute non persistent
        MessageOutput.printf("Limit Non-Persistent: %d W\r\n", payload_val);
        if (!properties.retain) {
            inv->sendActivePowerControlRequest(Hoymiles.getRadio(), payload_val, PowerLimitControlType::AbsolutNonPersistent);
        } else {
            MessageOutput.println("Ignored because retained");
        }

    } else if (!strcmp(setting, TOPIC_SUB_POWER)) {
        // Turn inverter on or off
        MessageOutput.printf("Set inverter power to: %d\r\n", payload_val);
        inv->sendPowerControlRequest(Hoymiles.getRadio(), payload_val > 0);

    } else if (!strcmp(setting, TOPIC_SUB_RESTART)) {
        // Restart inverter
        MessageOutput.printf("Restart inverter\r\n");
        if (!properties.retain && payload_val == 1) {
            inv->sendRestartControlRequest(Hoymiles.getRadio());
        } else {
            MessageOutput.println("Ignored because retained");
        }
    }
}
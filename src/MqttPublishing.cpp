// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"

MqttPublishingClass MqttPublishing;

void MqttPublishingClass::init()
{
}

void MqttPublishingClass::loop()
{
    if (!MqttSettings.getConnected() && Hoymiles.getRadio()->isIdle()) {
        return;
    }

    CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        MqttSettings.publish("dtu/uptime", String(millis() / 1000));
        MqttSettings.publish("dtu/ip", NetworkSettings.localIP().toString());

        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            char buffer[sizeof(uint64_t) * 8 + 1];
            sprintf(buffer, "%0lx%08lx",
                ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
            String subtopic = String(buffer);

            MqttSettings.publish(subtopic + "/name", inv->name());

            uint32_t lastUpdate = inv->Statistics()->getLastUpdate();
            if (lastUpdate > 0 && lastUpdate != _lastPublishStats[i]) {
                _lastPublishStats[i] = lastUpdate;

                // Loop all channels
                for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
                    for (uint8_t f = 0; f < sizeof(_publishFields); f++) {
                        publishField(inv, c, _publishFields[f]);
                    }
                }
            }

            yield();
        }

        _lastPublish = millis();
    }
}

void MqttPublishingClass::publishField(std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId)
{
    String topic = getTopic(inv, channel, fieldId);
    if (topic == "") {
        return;
    }

    MqttSettings.publish(topic, String(inv->Statistics()->getChannelFieldValue(channel, fieldId)));
}

String MqttPublishingClass::getTopic(std::shared_ptr<InverterAbstract> inv, uint8_t channel, uint8_t fieldId)
{
    if (!inv->Statistics()->hasChannelFieldValue(channel, fieldId)) {
        return String("");
    }

    char buffer[sizeof(uint64_t) * 8 + 1];
    sprintf(buffer, "%0lx%08lx",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
    String invSerial = String(buffer);

    String chanName;
    if (channel == 0 && fieldId == FLD_PDC) {
        chanName = "powerdc";
    } else {
        chanName = inv->Statistics()->getChannelFieldName(channel, fieldId);
        chanName.toLowerCase();
    }

    return invSerial + "/" + String(channel) + "/" + chanName;
}
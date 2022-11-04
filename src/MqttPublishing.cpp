// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <ctime>

MqttPublishingClass MqttPublishing;

void MqttPublishingClass::init()
{
}

void MqttPublishingClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.getRadio()->isIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {

        // compose subtopic "dtu/<serialnumber>" with config.DtuSerial as hex
        char dtu_subtopic[sizeof(uint64_t) * 8 + 5];
        snprintf(dtu_subtopic, sizeof(dtu_subtopic), "dtu/%0x%08x",
        ((uint32_t)((config.Dtu_Serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(config.Dtu_Serial & 0xFFFFFFFF)));

        MqttSettings.publish(String(dtu_subtopic) + "/uptime", String(millis() / 1000));
        MqttSettings.publish(String(dtu_subtopic) + "/ip", NetworkSettings.localIP().toString());
        MqttSettings.publish(String(dtu_subtopic) + "/hostname", NetworkSettings.getHostname());
        if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
            MqttSettings.publish(String(dtu_subtopic) + "/rssi", String(WiFi.RSSI()));
        }

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

    String chanName;
    if (channel == 0 && fieldId == FLD_PDC) {
        chanName = "powerdc";
    } else {
        chanName = inv->Statistics()->getChannelFieldName(channel, fieldId);
        chanName.toLowerCase();
    }

    return inv->serialString() + "/" + String(channel) + "/" + chanName;
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "ArduinoJson.h"
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <ctime>
#include <string>

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
        MqttSettings.publish("dtu/uptime", String(millis() / 1000));
        MqttSettings.publish("dtu/ip", NetworkSettings.localIP().toString());
        MqttSettings.publish("dtu/hostname", NetworkSettings.getHostname());
        if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
            MqttSettings.publish("dtu/rssi", String(WiFi.RSSI()));
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

                    // Publish maxpower and error code to Victron Venus OS
                    String deviceInstance = MqttSettings.getVictronDeviceInstance(subtopic);
                    String portalid = MqttSettings.getVictronPortalId();
                    if ( portalid == NULL ) { portalid = "NOportalId"; }

                    String Vtopic = "W/" + portalid + "/pvinverter/" + deviceInstance + "/ErrorCode";
                    DynamicJsonDocument val1Doc(128);
                    val1Doc["value"] = 0;
                    JsonObject val1Obj = val1Doc.as<JsonObject>();
                    String data1;
                    serializeJson(val1Obj, data1);
                    MqttSettings.publishVictron(Vtopic, data1);

                    Vtopic = "W/" + portalid + "/pvinverter/" + deviceInstance + "/Ac/MaxPower";
                    DynamicJsonDocument val2Doc(128);
                    val2Doc["value"] = maxpower;
                    JsonObject val2Obj = val2Doc.as<JsonObject>();
                    String data2;
                    serializeJson(val2Obj, data2);
                    MqttSettings.publishVictron(Vtopic, data2);
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
                    if (c > 0) {
                        INVERTER_CONFIG_T* inv_cfg = Configuration.getInverterConfig(inv->serial());
                        if (inv_cfg != nullptr) {
                            MqttSettings.publish(inv->serialString() + "/" + String(c) + "/name", inv_cfg->channel[c - 1].Name);
                        }
                    }
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

    // topic = "W/{}/pvinverter/{}/{}".format(portalId, deviceId, key) # UPDATE THIS
    // print("{} = {}".format(topic, data.get(key) ) )
    // client.publish(topic, json.dumps({ "value": data.get(key) }) )
    if (channel == 0) {

        float fieldvalue = float(inv->Statistics()->getChannelFieldValue(channel, fieldId));
        fieldvalue = round(fieldvalue * 100)/100;
        String fieldname = (inv->Statistics()->getChannelFieldName(channel, fieldId));
        String portalid = MqttSettings.getVictronPortalId();
        if ( portalid == NULL ) { portalid = "NOportalId"; }
        String topic = "W/" + portalid + "/pvinverter";
        String topic_Victron_sum;
        String topic_Victron_phase;
    
        char serial[sizeof(uint64_t) * 8 + 1];
        snprintf(serial, sizeof(serial), "%0x%08x",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
        String invSerial = String(serial);

        String deviceInstance = MqttSettings.getVictronDeviceInstance(serial);

        int response = false;
        if ( fieldname == "Voltage" ) { response = true; }
        if ( fieldname == "Power") { response = true; }
        if ( fieldname == "Current" ) { response = true; }
        if ( fieldname == "YieldTotal") { response = true; }

        if ( response ) {   
            // fieldname[0] = std::toupper(fieldname[0]);
            if ( fieldname == "YieldTotal" ) { 
                topic_Victron_sum += topic + "/" + deviceInstance + "/Ac/Energy/Forward";
                topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L1/Energy/Forward";
            } else {
                topic_Victron_sum += topic + "/" + deviceInstance + "/Ac/" + fieldname;
                topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L1/" + fieldname;
            }    

            DynamicJsonDocument valueDoc(256);
            valueDoc["value"] = fieldvalue;
            JsonObject valueObj = valueDoc.as<JsonObject>();

            String data;
            serializeJson(valueObj, data);

            Serial.print(F("MqTT publish value: "));
            Serial.print(data);
            Serial.print(F(" to Venus OS with topic: "));
            Serial.println(topic_Victron_sum);

            MqttSettings.publishVictron(topic_Victron_sum, data);
            MqttSettings.publishVictron(topic_Victron_phase, data);
        }
     }
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
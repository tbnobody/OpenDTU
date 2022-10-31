// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "ArduinoJson.h"
#include "MqttVictronPublishing.h"
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <ctime>
#include <string>

MqttVictronPublishingClass MqttVictronPublishing;

void MqttVictronPublishingClass::init()
{
}

void MqttVictronPublishingClass::loop()
{
    if (!Configuration.get().Mqtt_Victron_Enabled) {
        return;
    }

    if (!MqttSettings.getConnected() || !Hoymiles.getRadio()->isIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {

        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            char buffer[sizeof(uint64_t) * 8 + 1];
            snprintf(buffer, sizeof(buffer), "%0x%08x",
                ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
                ((uint32_t)(inv->serial() & 0xFFFFFFFF)));
            String str_serial = String(buffer);

            // Publish inverter as device service to Victron Venus OS with connected=1
            DynamicJsonDocument serviceDoc(256);
            serviceDoc[str_serial] = F("pvinverter");
            JsonObject serviceObj = serviceDoc.as<JsonObject>();

            // Get Current phase
            uint16_t invphase = config.Inverter[i+1].CurrentPhase;
            String invname = config.Inverter[i+1].Name;

            String Vtopic = ("device/HM" + str_serial + "/Status");
            DynamicJsonDocument rootDoc(1024);
            rootDoc[F("clientId")] = "HM" + str_serial;
            rootDoc[F("connected")] = 1;
            rootDoc[F("version")] = "0.1-L" + String(invphase) + "-" + invname;
            rootDoc[F("services")] = serviceObj;
            JsonObject rootObj = rootDoc.as<JsonObject>();
            String data;
            serializeJson(rootObj, data);
            MqttSettings.publishVictron(Vtopic, data);

            if (inv->SystemConfigPara()->getLastUpdate() > 0) {
                // Get Limit
                uint16_t maxpower = inv->DevInfo()->getMaxPower();

                // Publish limit and error code to Victron Venus OS
                String deviceInstance = MqttSettings.getVictronDeviceInstance(str_serial);
                String portalid = MqttSettings.getVictronPortalId();
                if ( portalid == NULL ) { portalid = "NOportalId"; }

                Vtopic = "W/" + portalid + "/pvinverter/" + deviceInstance + "/ErrorCode";
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

            uint32_t lastUpdate = inv->Statistics()->getLastUpdate();
            if (lastUpdate > 0 && lastUpdate != _lastPublishStats[i]) {
                _lastPublishStats[i] = lastUpdate;
                              
                // Loop all fields in channel 0
                for (uint8_t f = 0; f < sizeof(_publishFields); f++) {
                    publishField(inv, invphase, _publishFields[f]);
                }
            }

            yield();
        }

        _lastPublish = millis();
    }
}

void MqttVictronPublishingClass::publishField(std::shared_ptr<InverterAbstract> inv, uint16_t invphase, uint8_t fieldId)
{
    // topic = "W/{}/pvinverter/{}/{}".format(portalId, deviceId, key)
    // print("{} = {}".format(topic, data.get(key) ) )
    // client.publish(topic, json.dumps({ "value": data.get(key) }) )
    float fieldvalue = float(inv->Statistics()->getChannelFieldValue(0, fieldId));
    fieldvalue = round(fieldvalue * 100)/100;
    String fieldname = (inv->Statistics()->getChannelFieldName(0, fieldId));
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
            topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L" + invphase + "/Energy/Forward";
        } else {
            topic_Victron_sum += topic + "/" + deviceInstance + "/Ac/" + fieldname;
            topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L" + invphase + "/" + fieldname;
        }    

        DynamicJsonDocument valueDoc(256);
        valueDoc["value"] = fieldvalue;
        JsonObject valueObj = valueDoc.as<JsonObject>();

        String data;
        serializeJson(valueObj, data);

        Serial.print(F("MqTT publish value: "));
        Serial.print(data);
        Serial.print(F(" to Venus OS with topic sum: "));
        Serial.print(topic_Victron_sum);
        Serial.print(F(" and topic phase: "));
        Serial.println(topic_Victron_phase);

        MqttSettings.publishVictron(topic_Victron_sum, data);
        MqttSettings.publishVictron(topic_Victron_phase, data);
     }
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "ArduinoJson.h"
#include "MqttVictronPublishing.h"
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"

MqttVictronPublishingClass MqttVictronPublishing;

void MqttVictronPublishingClass::init()
{
}

void MqttVictronPublishingClass::loop()
{
    if (!Configuration.get().Mqtt_Victron_Enabled) {
        // Victron stuff enabled?
        return;
    }

    if (!Hoymiles.getRadio()->isIdle()) {
        // Hoymiles are ready to go?
        return;
    }

    if (MqttSettings.getConnected() && _registerForced) {
        // Connection established, force register inverter @Victron Cerbo
        registerInverter();
        _registerForced = false;
    }

    if (MqttSettings.getConnected() && !_wasConnected) {
        // Connection established, register inverter @Victron Cerbo
        _wasConnected = true;
        registerInverter();
    } else if (!MqttSettings.getConnected() && _wasConnected) {
        // Connection lost, register again next time when connection established
        _wasConnected = false;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {

        // Loop all inverters
        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
            auto inv = Hoymiles.getInverterByPos(i);

            String str_serial = inv->serialString();
 
            uint32_t lastUpdate = inv->Statistics()->getLastUpdate();
            if (lastUpdate > 0 && lastUpdate != _lastPublishStats[i]) {
                _lastPublishStats[i] = lastUpdate;
 
                String invname = inv->name();

                // Get Current phase
                uint8_t invphase = config.Inverter[i].CurrentPhase;

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

void MqttVictronPublishingClass::forceRegister()
{
    _registerForced = true;
}

void MqttVictronPublishingClass::registerInverter()
{
    const CONFIG_T& config = Configuration.get();

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        String str_serial = inv->serialString();
        String invname = inv->name();

        // Get Current phase
        uint8_t invphase = config.Inverter[i].CurrentPhase;
        uint8_t invconnected;

        // Current phase = L0 --> No register to Victron Cerbo, only L1, L2, L3
        if (invphase == 0) { invconnected = 0; } else { invconnected = 1; }

        // Publish inverter as device service to Victron Venus OS with connected=1
        DynamicJsonDocument serviceDoc(64);
        serviceDoc[str_serial] = F("pvinverter");
        JsonObject serviceObj = serviceDoc.as<JsonObject>();

        String Vtopic = ("device/HM" + str_serial + "/Status");
        DynamicJsonDocument rootDoc(256);
        rootDoc[F("clientId")] = "HM" + str_serial;
        rootDoc[F("connected")] = invconnected;
        rootDoc[F("version")] = "0.1-L" + String(invphase) + "-" + invname;
        rootDoc[F("services")] = serviceObj;
        JsonObject rootObj = rootDoc.as<JsonObject>();
        String data;
        serializeJson(rootObj, data);
        MqttSettings.publishVictron(Vtopic, data);

        // Publish limit and error code to Victron Venus OS
        uint16_t maxpower = inv->DevInfo()->getMaxPower();

        String deviceInstance = MqttSettings.getVictronDeviceInstance(str_serial);
        String portalid = MqttSettings.getVictronPortalId();
        if ( portalid == NULL ) { portalid = "NOportalId"; }

        String Vtopic = "W/" + portalid + "/pvinverter/" + deviceInstance + "/ErrorCode";
        DynamicJsonDocument val1Doc(32);
        val1Doc["value"] = 0;
        JsonObject val1Obj = val1Doc.as<JsonObject>();
        String data1;
        serializeJson(val1Obj, data1);
        MqttSettings.publishVictron(Vtopic, data1);

        Vtopic = "W/" + portalid + "/pvinverter/" + deviceInstance + "/Ac/MaxPower";
        DynamicJsonDocument val2Doc(32);
        val2Doc["value"] = maxpower;
        JsonObject val2Obj = val2Doc.as<JsonObject>();
        String data2;
        serializeJson(val2Obj, data2);
        MqttSettings.publishVictron(Vtopic, data2);
    }
}

void MqttVictronPublishingClass::publishField(std::shared_ptr<InverterAbstract> inv, uint8_t invphase, uint8_t fieldId)
{
    bool response = false;
    String fieldname;
    if ( fieldId == 4 ) { response = true; fieldname = "YieldTotal"; }
    if ( fieldId == 5 ) { response = true; fieldname = "Voltage"; }
    if ( fieldId == 6 ) { response = true; fieldname = "Current"; }
    if ( fieldId == 7 ) { response = true; fieldname = "Power"; }

    if ( response ) {   
        double fieldval = double(inv->Statistics()->getChannelFieldValue(0, fieldId));
        double fieldvalue = floor( fieldval * 100.0 + .5 ) / 100.0;

        String portalid = MqttSettings.getVictronPortalId();
        if ( portalid == NULL ) { portalid = "NOportalId"; }
        String topic = "W/" + portalid + "/pvinverter";
        String topic_Victron_sum;
        String topic_Victron_phase;
    
        String invSerial = inv->serialString();

        String deviceInstance = MqttSettings.getVictronDeviceInstance(invSerial);

        if ( fieldId == 4 ) { 
            topic_Victron_sum += topic + "/" + deviceInstance + "/Ac/Energy/Forward";
            topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L" + invphase + "/Energy/Forward";
        } else {
            topic_Victron_sum += topic + "/" + deviceInstance + "/Ac/" + fieldname;
            topic_Victron_phase += topic + "/" + deviceInstance + "/Ac/L" + invphase + "/" + fieldname;
        }    

        DynamicJsonDocument valueDoc(32);
        valueDoc["value"] = fieldvalue;
        JsonObject valueObj = valueDoc.as<JsonObject>();

        String data;
        serializeJson(valueObj, data);

        Serial.print(F("MqTT publish value: "));
        Serial.print(fieldId);
        Serial.print(data);
        Serial.print(F(" to Venus OS with topic sum: "));
        Serial.print(topic_Victron_sum);
        Serial.print(F(" and topic phase: "));
        Serial.println(topic_Victron_phase);

        MqttSettings.publishVictron(topic_Victron_sum, data);
        MqttSettings.publishVictron(topic_Victron_phase, data);

        // Send Value 0 to other current phases and energy forward
        uint8_t nonval = 0;
        DynamicJsonDocument nonvalueDoc(32);
        nonvalueDoc["value"] = nonval;
        JsonObject nonvalueObj = nonvalueDoc.as<JsonObject>();

        String nondata;
        serializeJson(nonvalueObj, nondata);

        String valA, valB, topicA, topicB;

        switch ( invphase ) {
            case 1:
                valA = "L2";
                valB = "L3";
                break;
            case 2:
                valA = "L1";
                valB = "L3";
                break;
            case 3:
                valA = "L1";
                valB = "L2";
                break;
            default:
                return;
                break;
        }

        if ( fieldname == "YieldTotal" ) { 
                topicA += topic + "/" + deviceInstance + "/Ac/" + valA + "/Energy/Forward";      
                topicB += topic + "/" + deviceInstance + "/Ac/" + valB + "/Energy/Forward"; 
        } else {
                topicA += topic + "/" + deviceInstance + "/Ac/" + valA + "/" + fieldname;
                topicB += topic + "/" + deviceInstance + "/Ac/" + valB + "/" + fieldname;
        }
        
        MqttSettings.publishVictron(topicA, nondata);
        MqttSettings.publishVictron(topicB, nondata);
    }
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "MqttVedirectPublishing.h"
#include "VeDirectFrameHandler.h"
#include "MqttSettings.h"



MqttVedirectPublishingClass MqttVedirectPublishing;

void MqttVedirectPublishingClass::init()
{
    Serial2.begin(19200, SERIAL_8N1, VICTRON_PIN_RX, VICTRON_PIN_TX);
    Serial2.flush();
}

void MqttVedirectPublishingClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() && !config.Vedirect_Enabled) {
        return;
    }    

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        String key;
        String value;
        bool bChanged;
        unsigned long now = millis();

        while ( Serial2.available() && ((millis() - now) < 500)) {
            _myve.rxData(Serial2.read());
        }
        yield();

        String topic = "";
        for ( int i = 0; i < _myve.veEnd; i++ ) {
            key = _myve.veName[i];
            value = _myve.veValue[i];
            
            // just for debug
            Serial.print(key.c_str());
            Serial.print("= ");
            Serial.println(value.c_str()); 

            // Add new key, value pairs to map and update changed values.
            // Mark changed values
            auto a = _kv_map.find(key);
            if (a !=  _kv_map.end()) {
                if (_kv_map[key] == value) {
                    bChanged = false;
                }
                else {
                    _kv_map[key] = value;
                    bChanged = true;
                }
            }
            else {	
                _kv_map.insert(std::make_pair(key, value));
                bChanged = true;
            }
        
            // publish only changed key, values pairs
            if (!config.Vedirect_UpdatesOnly || (bChanged && config.Vedirect_UpdatesOnly)) {
                topic = "victron/";
                topic.concat(key);  
                topic.replace("#",""); // # is a no go in mqtt topic
                MqttSettings.publish(topic.c_str(), value.c_str()); 
            }
        }
        _lastPublish = millis();
    }
}
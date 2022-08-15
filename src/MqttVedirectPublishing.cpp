// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VeDirectFrameHandler.h"
#include "MqttVedirectPublishing.h"
#include "MqttSettings.h"




MqttVedirectPublishingClass MqttVedirectPublishing;

void MqttVedirectPublishingClass::init()
{
}

void MqttVedirectPublishingClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() || !config.Vedirect_Enabled) {
        return;
    }    

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        String key;
        String value;
        bool bChanged;

        String topic = "";
        for ( int i = 0; i < VeDirect.veEnd; i++ ) {
            key = VeDirect.veName[i];
            value = VeDirect.veValue[i];
            
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
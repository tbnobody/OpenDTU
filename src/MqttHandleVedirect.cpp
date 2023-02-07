// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VeDirectFrameHandler.h"
#include "MqttHandleVedirect.h"
#include "MqttSettings.h"




MqttHandleVedirectClass MqttHandleVedirect;

void MqttHandleVedirectClass::init()
{
}

void MqttHandleVedirectClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() || !config.Vedirect_Enabled) {
        return;
    }   

    String serial;
    auto pos = VeDirect.veMap.find("SER");
    if (pos == VeDirect.veMap.end()) {
        return;
    } 
    else {
        serial = pos->second;
    }

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        String key;
        String value;
        bool bChanged;

        String topic = "";
        for (auto it = VeDirect.veMap.begin(); it != VeDirect.veMap.end(); ++it) {
            key = it->first;
            value = it->second;
            
            // Mark changed values
            auto a = _kv_map.find(key);
            bChanged = true;
            if (a !=  _kv_map.end()) {
                if (_kv_map[key] == value) {
                    bChanged = false;
                }   
            }
        
            // publish only changed key, values pairs
            if (!config.Vedirect_UpdatesOnly || (bChanged && config.Vedirect_UpdatesOnly)) {
                topic = "victron/" + serial + "/";
                topic.concat(key);  
                MqttSettings.publish(topic.c_str(), value.c_str()); 
            }
        }
        _kv_map = VeDirect.veMap;
        _lastPublish = millis();
    }
}
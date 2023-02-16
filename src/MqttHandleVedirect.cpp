// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VeDirectFrameHandler.h"
#include "MqttHandleVedirect.h"
#include "MqttSettings.h"
#include "MessageOutput.h"




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

    if (!VeDirect.isDataValid()) { 
        MessageOutput.printf("VeDirect Data not valid: Stopping publishing. Last read before %lu seconds\r\n", (millis() - VeDirect.getLastUpdate()) / 1000);
        return;
    }

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        String key;
        String value;
        String mapedValue;
        bool bChanged = false;
        String serial = VeDirect.veMap["SER"];

        String topic = "";
        for (auto it = VeDirect.veMap.begin(); it != VeDirect.veMap.end(); ++it) {
            key = it->first;
            value = it->second;
            
            if (config.Vedirect_UpdatesOnly){
                // Mark changed values
                auto a = _kv_map.find(key);
                bChanged = true;
                if (a !=  _kv_map.end()) {
                    if (a->first.equals(value)) {
                        bChanged = false;
                    }   
                }
            }

            // publish only changed key, values pairs
            if (!config.Vedirect_UpdatesOnly || (bChanged && config.Vedirect_UpdatesOnly)) {
                topic = "victron/" + serial + "/";
                topic.concat(key);
                if (key.equals("PID")) {
                    mapedValue = VeDirect.getPidAsString(value.c_str());
                } 
                else if (key.equals("CS")) {
                    mapedValue = VeDirect.getCsAsString(value.c_str());
                } 
                else if (key.equals("ERR")) {
                    mapedValue = VeDirect.getErrAsString(value.c_str());
                } 
                else if (key.equals("OR")) {
                    mapedValue = VeDirect.getOrAsString(value.c_str());
                } 
                else if (key.equals("MPPT")) {
                    mapedValue = VeDirect.getMpptAsString(value.c_str());
                } 
                else if (key.equals("V") ||
                         key.equals("I") ||
                         key.equals("VPV")) {
                    mapedValue = round(value.toDouble() / 10.0) / 100.0;
                } 
                else if (key.equals("H19") ||
                         key.equals("H20") ||
                         key.equals("H22")) {
                    mapedValue = value.toDouble() / 100.0;
                } 
                else {
                    mapedValue = value;
                }
                MqttSettings.publish(topic.c_str(), mapedValue.c_str()); 
            }
        }
        if (config.Vedirect_UpdatesOnly){
            _kv_map = VeDirect.veMap;
        }
        _lastPublish = millis();
    }
}
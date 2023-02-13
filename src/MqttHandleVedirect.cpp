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

    String serial;
    auto pos = VeDirect.veMap.find("SER");
    if (pos == VeDirect.veMap.end()) {
        return;
    } 
    else {
        serial = pos->second;
    }

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        if (millis() - VeDirect.getLastUpdate() > (config.Vedirect_PollInterval * 3 * 1000)) {
            MessageOutput.printf("VeDirect Data too old: Stopping publishing. Last read before %f seconds\r\n", (millis() - VeDirect.getLastUpdate()) / 1000.0);
            return;
        }

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
                if (key.equals("PID")) {
                    value = VeDirect.getPidAsString(value.c_str());
                } 
                else if (key.equals("CS")) {
                    value = VeDirect.getCsAsString(value.c_str());
                } 
                else if (key.equals("ERR")) {
                    value = VeDirect.getErrAsString(value.c_str());
                } 
                else if (key.equals("OR")) {
                    value = VeDirect.getOrAsString(value.c_str());
                } 
                else if (key.equals("MPPT")) {
                    value = VeDirect.getMpptAsString(value.c_str());
                } 
                else if (key.equals("V") ||
                         key.equals("I") ||
                         key.equals("VPV")) {
                    value = round(value.toDouble() / 10.0) / 100.0;
                } 
                else if (key.equals("H19") ||
                         key.equals("H20") ||
                         key.equals("H22")) {
                    value = value.toDouble() / 100.0;
                } 
                MqttSettings.publish(topic.c_str(), value.c_str()); 
            }
        }
        _kv_map = VeDirect.veMap;
        _lastPublish = millis();
    }
}
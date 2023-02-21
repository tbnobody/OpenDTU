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
        return;
    }

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        String value;
        String topic = "victron/";
        topic.concat(VeDirect.veFrame.SER);
        topic.concat("/");

        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.PID != _kvFrame.PID)
            MqttSettings.publish(topic + "PID", VeDirect.getPidAsString(VeDirect.veFrame.PID)); 
        if (!config.Vedirect_UpdatesOnly || strcmp(VeDirect.veFrame.SER, _kvFrame.SER) != 0)
            MqttSettings.publish(topic + "SER", VeDirect.veFrame.SER ); 
        if (!config.Vedirect_UpdatesOnly || strcmp(VeDirect.veFrame.FW, _kvFrame.FW) != 0)
            MqttSettings.publish(topic + "FW", VeDirect.veFrame.FW); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.LOAD != _kvFrame.LOAD)
            MqttSettings.publish(topic + "LOAD", VeDirect.veFrame.LOAD == true ? "ON": "OFF"); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.CS != _kvFrame.CS)
            MqttSettings.publish(topic + "CS", VeDirect.getCsAsString(VeDirect.veFrame.CS)); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.ERR != _kvFrame.ERR)
            MqttSettings.publish(topic + "ERR", VeDirect.getErrAsString(VeDirect.veFrame.ERR)); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.OR != _kvFrame.OR)
            MqttSettings.publish(topic + "OR", VeDirect.getOrAsString(VeDirect.veFrame.OR)); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.MPPT != _kvFrame.MPPT)
            MqttSettings.publish(topic + "MPPT", VeDirect.getMpptAsString(VeDirect.veFrame.MPPT)); 
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.HSDS != _kvFrame.HSDS) {
            value = VeDirect.veFrame.HSDS;
            MqttSettings.publish(topic + "HSDS", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.V != _kvFrame.V) {
            value = VeDirect.veFrame.V;
            MqttSettings.publish(topic + "V", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.I != _kvFrame.I) {
            value = VeDirect.veFrame.I;
            MqttSettings.publish(topic + "I", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.VPV != _kvFrame.VPV) {
            value = VeDirect.veFrame.VPV;
            MqttSettings.publish(topic + "VPV", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.PPV != _kvFrame.PPV) {
            value = VeDirect.veFrame.PPV;
            MqttSettings.publish(topic + "PPV", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.H19 != _kvFrame.H19) {
            value = VeDirect.veFrame.H19;
            MqttSettings.publish(topic + "H19", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.H20 != _kvFrame.H20) {
            value = VeDirect.veFrame.H20;
            MqttSettings.publish(topic + "H20", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.H21 != _kvFrame.H21) {
            value = VeDirect.veFrame.H21;
            MqttSettings.publish(topic + "H21", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.H22 != _kvFrame.H22) {
            value = VeDirect.veFrame.H22;
            MqttSettings.publish(topic + "H22", value); 
        }
        if (!config.Vedirect_UpdatesOnly || VeDirect.veFrame.H23 != _kvFrame.H23) {
            value = VeDirect.veFrame.H23;
            MqttSettings.publish(topic + "H23", value); 
        }
        if (config.Vedirect_UpdatesOnly){
            _kvFrame= VeDirect.veFrame;
        }
        _lastPublish = millis();
    }
}
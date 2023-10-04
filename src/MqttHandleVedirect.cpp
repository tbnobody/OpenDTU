// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VeDirectMpptController.h"
#include "MqttHandleVedirect.h"
#include "MqttSettings.h"
#include "MessageOutput.h"




MqttHandleVedirectClass MqttHandleVedirect;

// #define MQTTHANDLEVEDIRECT_DEBUG

void MqttHandleVedirectClass::init()
{
    // initially force a full publish
    _nextPublishUpdatesOnly = 0;
    _nextPublishFull = 1;
}

void MqttHandleVedirectClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() || !config.Vedirect_Enabled) {
        return;
    }

    if (!VeDirectMppt.isDataValid()) {
        return;
    }

    if ((millis() >= _nextPublishFull) || (millis() >= _nextPublishUpdatesOnly)) {
        // determine if this cycle should publish full values or updates only
        if (_nextPublishFull <= _nextPublishUpdatesOnly) {
            _PublishFull = true;
        } else {
            _PublishFull = !config.Vedirect_UpdatesOnly;
        }

        #ifdef MQTTHANDLEVEDIRECT_DEBUG
        MessageOutput.printf("\r\n\r\nMqttHandleVedirectClass::loop millis %lu   _nextPublishUpdatesOnly %u   _nextPublishFull %u\r\n", millis(), _nextPublishUpdatesOnly, _nextPublishFull);
        if (_PublishFull) {
            MessageOutput.println("MqttHandleVedirectClass::loop publish full");
        } else {
            MessageOutput.println("MqttHandleVedirectClass::loop publish updates only");
        }
        #endif

        String value;
        String topic = "victron/";
        topic.concat(VeDirectMppt.veFrame.SER);
        topic.concat("/");

        if (_PublishFull || VeDirectMppt.veFrame.PID != _kvFrame.PID)
            MqttSettings.publish(topic + "PID", VeDirectMppt.getPidAsString(VeDirectMppt.veFrame.PID));
        if (_PublishFull || strcmp(VeDirectMppt.veFrame.SER, _kvFrame.SER) != 0)
            MqttSettings.publish(topic + "SER", VeDirectMppt.veFrame.SER );
        if (_PublishFull || strcmp(VeDirectMppt.veFrame.FW, _kvFrame.FW) != 0)
            MqttSettings.publish(topic + "FW", VeDirectMppt.veFrame.FW);
        if (_PublishFull || VeDirectMppt.veFrame.LOAD != _kvFrame.LOAD)
            MqttSettings.publish(topic + "LOAD", VeDirectMppt.veFrame.LOAD == true ? "ON": "OFF");
        if (_PublishFull || VeDirectMppt.veFrame.CS != _kvFrame.CS)
            MqttSettings.publish(topic + "CS", VeDirectMppt.getCsAsString(VeDirectMppt.veFrame.CS));
        if (_PublishFull || VeDirectMppt.veFrame.ERR != _kvFrame.ERR)
            MqttSettings.publish(topic + "ERR", VeDirectMppt.getErrAsString(VeDirectMppt.veFrame.ERR));
        if (_PublishFull || VeDirectMppt.veFrame.OR != _kvFrame.OR)
            MqttSettings.publish(topic + "OR", VeDirectMppt.getOrAsString(VeDirectMppt.veFrame.OR));
        if (_PublishFull || VeDirectMppt.veFrame.MPPT != _kvFrame.MPPT)
            MqttSettings.publish(topic + "MPPT", VeDirectMppt.getMpptAsString(VeDirectMppt.veFrame.MPPT));
        if (_PublishFull || VeDirectMppt.veFrame.HSDS != _kvFrame.HSDS) {
            value = VeDirectMppt.veFrame.HSDS;
            MqttSettings.publish(topic + "HSDS", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.V != _kvFrame.V) {
            value = VeDirectMppt.veFrame.V;
            MqttSettings.publish(topic + "V", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.I != _kvFrame.I) {
            value = VeDirectMppt.veFrame.I;
            MqttSettings.publish(topic + "I", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.P != _kvFrame.P) {
            value = VeDirectMppt.veFrame.P;
            MqttSettings.publish(topic + "P", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.VPV != _kvFrame.VPV) {
            value = VeDirectMppt.veFrame.VPV;
            MqttSettings.publish(topic + "VPV", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.IPV != _kvFrame.IPV) {
            value = VeDirectMppt.veFrame.IPV;
            MqttSettings.publish(topic + "IPV", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.PPV != _kvFrame.PPV) {
            value = VeDirectMppt.veFrame.PPV;
            MqttSettings.publish(topic + "PPV", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.E != _kvFrame.E) {
            value = VeDirectMppt.veFrame.E;
            MqttSettings.publish(topic + "E", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.H19 != _kvFrame.H19) {
            value = VeDirectMppt.veFrame.H19;
            MqttSettings.publish(topic + "H19", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.H20 != _kvFrame.H20) {
            value = VeDirectMppt.veFrame.H20;
            MqttSettings.publish(topic + "H20", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.H21 != _kvFrame.H21) {
            value = VeDirectMppt.veFrame.H21;
            MqttSettings.publish(topic + "H21", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.H22 != _kvFrame.H22) {
            value = VeDirectMppt.veFrame.H22;
            MqttSettings.publish(topic + "H22", value);
        }
        if (_PublishFull || VeDirectMppt.veFrame.H23 != _kvFrame.H23) {
            value = VeDirectMppt.veFrame.H23;
            MqttSettings.publish(topic + "H23", value);
        }
        if (!_PublishFull) {
            _kvFrame= VeDirectMppt.veFrame;
        }

        // now calculate next points of time to publish
        _nextPublishUpdatesOnly = millis() + (config.Mqtt_PublishInterval * 1000);

        if (_PublishFull) {
            // when Home Assistant MQTT-Auto-Discovery is active,
            // and "enable expiration" is active, all values must be published at
            // least once before the announced expiry interval is reached
            if ((config.Vedirect_UpdatesOnly) && (config.Mqtt_Hass_Enabled) && (config.Mqtt_Hass_Expire)) {
                _nextPublishFull = millis() + (((config.Mqtt_PublishInterval * 3) - 1) * 1000);

                #ifdef MQTTHANDLEVEDIRECT_DEBUG
                uint32_t _tmpNextFullSeconds = (config.Mqtt_PublishInterval * 3) - 1;
                MessageOutput.printf("MqttHandleVedirectClass::loop _tmpNextFullSeconds %u - _nextPublishFull %u \r\n", _tmpNextFullSeconds, _nextPublishFull);
                #endif

            } else {
                // no future publish full needed
                _nextPublishFull = UINT32_MAX;
            }
        }

        #ifdef MQTTHANDLEVEDIRECT_DEBUG
        MessageOutput.printf("MqttHandleVedirectClass::loop _nextPublishUpdatesOnly %u   _nextPublishFull %u\r\n", _nextPublishUpdatesOnly, _nextPublishFull);
        #endif
    }
}
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Helge Erbe and others
 */
#include "VictronMppt.h"
#include "MqttHandleVedirect.h"
#include "MqttSettings.h"
#include "MessageOutput.h"




MqttHandleVedirectClass MqttHandleVedirect;

// #define MQTTHANDLEVEDIRECT_DEBUG

void MqttHandleVedirectClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleVedirectClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    // initially force a full publish
    this->forceUpdate();
}

void MqttHandleVedirectClass::forceUpdate()
{
    // initially force a full publish
    _nextPublishUpdatesOnly = 0;
    _nextPublishFull = 1;
}


void MqttHandleVedirectClass::loop()
{
    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected() || !config.Vedirect.Enabled) {
        return;
    }

    if (!VictronMppt.isDataValid()) {
        return;
    }

    if ((millis() >= _nextPublishFull) || (millis() >= _nextPublishUpdatesOnly)) {
        // determine if this cycle should publish full values or updates only
        if (_nextPublishFull <= _nextPublishUpdatesOnly) {
            _PublishFull = true;
        } else {
            _PublishFull = !config.Vedirect.UpdatesOnly;
        }

        #ifdef MQTTHANDLEVEDIRECT_DEBUG
        MessageOutput.printf("\r\n\r\nMqttHandleVedirectClass::loop millis %lu   _nextPublishUpdatesOnly %u   _nextPublishFull %u\r\n", millis(), _nextPublishUpdatesOnly, _nextPublishFull);
        if (_PublishFull) {
            MessageOutput.println("MqttHandleVedirectClass::loop publish full");
        } else {
            MessageOutput.println("MqttHandleVedirectClass::loop publish updates only");
        }
        #endif

        auto spMpptData = VictronMppt.getData();
        String value;
        String topic = "victron/";
        topic.concat(spMpptData->SER);
        topic.concat("/");

        if (_PublishFull || spMpptData->PID != _kvFrame.PID)
            MqttSettings.publish(topic + "PID", spMpptData->getPidAsString());
        if (_PublishFull || strcmp(spMpptData->SER, _kvFrame.SER) != 0)
            MqttSettings.publish(topic + "SER", spMpptData->SER );
        if (_PublishFull || strcmp(spMpptData->FW, _kvFrame.FW) != 0)
            MqttSettings.publish(topic + "FW", spMpptData->FW);
        if (_PublishFull || spMpptData->LOAD != _kvFrame.LOAD)
            MqttSettings.publish(topic + "LOAD", spMpptData->LOAD == true ? "ON": "OFF");
        if (_PublishFull || spMpptData->CS != _kvFrame.CS)
            MqttSettings.publish(topic + "CS", spMpptData->getCsAsString());
        if (_PublishFull || spMpptData->ERR != _kvFrame.ERR)
            MqttSettings.publish(topic + "ERR", spMpptData->getErrAsString());
        if (_PublishFull || spMpptData->OR != _kvFrame.OR)
            MqttSettings.publish(topic + "OR", spMpptData->getOrAsString());
        if (_PublishFull || spMpptData->MPPT != _kvFrame.MPPT)
            MqttSettings.publish(topic + "MPPT", spMpptData->getMpptAsString());
        if (_PublishFull || spMpptData->HSDS != _kvFrame.HSDS) {
            value = spMpptData->HSDS;
            MqttSettings.publish(topic + "HSDS", value);
        }
        if (_PublishFull || spMpptData->V != _kvFrame.V) {
            value = spMpptData->V;
            MqttSettings.publish(topic + "V", value);
        }
        if (_PublishFull || spMpptData->I != _kvFrame.I) {
            value = spMpptData->I;
            MqttSettings.publish(topic + "I", value);
        }
        if (_PublishFull || spMpptData->P != _kvFrame.P) {
            value = spMpptData->P;
            MqttSettings.publish(topic + "P", value);
        }
        if (_PublishFull || spMpptData->VPV != _kvFrame.VPV) {
            value = spMpptData->VPV;
            MqttSettings.publish(topic + "VPV", value);
        }
        if (_PublishFull || spMpptData->IPV != _kvFrame.IPV) {
            value = spMpptData->IPV;
            MqttSettings.publish(topic + "IPV", value);
        }
        if (_PublishFull || spMpptData->PPV != _kvFrame.PPV) {
            value = spMpptData->PPV;
            MqttSettings.publish(topic + "PPV", value);
        }
        if (_PublishFull || spMpptData->E != _kvFrame.E) {
            value = spMpptData->E;
            MqttSettings.publish(topic + "E", value);
        }
        if (_PublishFull || spMpptData->H19 != _kvFrame.H19) {
            value = spMpptData->H19;
            MqttSettings.publish(topic + "H19", value);
        }
        if (_PublishFull || spMpptData->H20 != _kvFrame.H20) {
            value = spMpptData->H20;
            MqttSettings.publish(topic + "H20", value);
        }
        if (_PublishFull || spMpptData->H21 != _kvFrame.H21) {
            value = spMpptData->H21;
            MqttSettings.publish(topic + "H21", value);
        }
        if (_PublishFull || spMpptData->H22 != _kvFrame.H22) {
            value = spMpptData->H22;
            MqttSettings.publish(topic + "H22", value);
        }
        if (_PublishFull || spMpptData->H23 != _kvFrame.H23) {
            value = spMpptData->H23;
            MqttSettings.publish(topic + "H23", value);
        }
        if (!_PublishFull) {
            _kvFrame = *spMpptData;
        }

        // now calculate next points of time to publish
        _nextPublishUpdatesOnly = millis() + (config.Mqtt.PublishInterval * 1000);

        if (_PublishFull) {
            // when Home Assistant MQTT-Auto-Discovery is active,
            // and "enable expiration" is active, all values must be published at
            // least once before the announced expiry interval is reached
            if ((config.Vedirect.UpdatesOnly) && (config.Mqtt.Hass.Enabled) && (config.Mqtt.Hass.Expire)) {
                _nextPublishFull = millis() + (((config.Mqtt.PublishInterval * 3) - 1) * 1000);

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
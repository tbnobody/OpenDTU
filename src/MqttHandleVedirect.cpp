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
    _loopTask.setCallback([this] { loop(); });
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

        for (int idx = 0; idx < VictronMppt.controllerAmount(); ++idx) {
            std::optional<VeDirectMpptController::data_t> optMpptData = VictronMppt.getData(idx);
            if (!optMpptData.has_value()) { continue; }

            auto const& kvFrame = _kvFrames[optMpptData->SER];
            publish_mppt_data(*optMpptData, kvFrame);
            if (!_PublishFull) {
                _kvFrames[optMpptData->SER] = *optMpptData;
            }
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

void MqttHandleVedirectClass::publish_mppt_data(const VeDirectMpptController::data_t &currentData,
                                                const VeDirectMpptController::data_t &previousData) const {
    String value;
    String topic = "victron/";
    topic.concat(currentData.SER);
    topic.concat("/");

    if (_PublishFull || currentData.PID != previousData.PID) {
        MqttSettings.publish(topic + "PID", currentData.getPidAsString().data());
    }

    if (_PublishFull || strcmp(currentData.SER, previousData.SER) != 0) {
        MqttSettings.publish(topic + "SER", currentData.SER);
    }

    if (_PublishFull || strcmp(currentData.FW, previousData.FW) != 0) {
        MqttSettings.publish(topic + "FW", currentData.FW);
    }

    if (_PublishFull || currentData.LOAD != previousData.LOAD) {
        MqttSettings.publish(topic + "LOAD", currentData.LOAD ? "ON" : "OFF");
    }

    if (_PublishFull || currentData.CS != previousData.CS) {
        MqttSettings.publish(topic + "CS", currentData.getCsAsString().data());
    }

    if (_PublishFull || currentData.ERR != previousData.ERR) {
        MqttSettings.publish(topic + "ERR", currentData.getErrAsString().data());
    }

    if (_PublishFull || currentData.OR != previousData.OR) {
        MqttSettings.publish(topic + "OR", currentData.getOrAsString().data());
    }

    if (_PublishFull || currentData.MPPT != previousData.MPPT) {
        MqttSettings.publish(topic + "MPPT", currentData.getMpptAsString().data());
    }

    if (_PublishFull || currentData.HSDS != previousData.HSDS) {
        MqttSettings.publish(topic + "HSDS", String(currentData.HSDS));
    }

    if (_PublishFull || currentData.V != previousData.V) {
        MqttSettings.publish(topic + "V", String(currentData.V));
    }

    if (_PublishFull || currentData.I != previousData.I) {
        MqttSettings.publish(topic + "I", String(currentData.I));
    }

    if (_PublishFull || currentData.P != previousData.P) {
        MqttSettings.publish(topic + "P", String(currentData.P));
    }

    if (_PublishFull || currentData.VPV != previousData.VPV) {
        MqttSettings.publish(topic + "VPV", String(currentData.VPV));
    }

    if (_PublishFull || currentData.IPV != previousData.IPV) {
        MqttSettings.publish(topic + "IPV", String(currentData.IPV));
    }

    if (_PublishFull || currentData.PPV != previousData.PPV) {
        MqttSettings.publish(topic + "PPV", String(currentData.PPV));
    }

    if (_PublishFull || currentData.E != previousData.E) {
        MqttSettings.publish(topic + "E", String(currentData.E));
    }

    if (_PublishFull || currentData.H19 != previousData.H19) {
        MqttSettings.publish(topic + "H19", String(currentData.H19));
    }

    if (_PublishFull || currentData.H20 != previousData.H20) {
        MqttSettings.publish(topic + "H20", String(currentData.H20));
    }

    if (_PublishFull || currentData.H21 != previousData.H21) {
        MqttSettings.publish(topic + "H21", String(currentData.H21));
    }

    if (_PublishFull || currentData.H22 != previousData.H22) {
        MqttSettings.publish(topic + "H22", String(currentData.H22));
    }

    if (_PublishFull || currentData.H23 != previousData.H23) {
        MqttSettings.publish(topic + "H23", String(currentData.H23));
    }
}

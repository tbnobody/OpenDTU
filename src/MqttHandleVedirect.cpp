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

#define PUBLISH(sm, t, val) \
    if (_PublishFull || currentData.sm != previousData.sm) { \
        MqttSettings.publish(topic + t, String(val)); \
    }

    PUBLISH(PID,   "PID",  currentData.getPidAsString().data());
    PUBLISH(SER,   "SER",  currentData.SER);
    PUBLISH(FW,    "FW",   currentData.FW);
    PUBLISH(LOAD,  "LOAD", (currentData.LOAD ? "ON" : "OFF"));
    PUBLISH(CS,    "CS",   currentData.getCsAsString().data());
    PUBLISH(ERR,   "ERR",  currentData.getErrAsString().data());
    PUBLISH(OR,    "OR",   currentData.getOrAsString().data());
    PUBLISH(MPPT,  "MPPT", currentData.getMpptAsString().data());
    PUBLISH(HSDS,  "HSDS", currentData.HSDS);
    PUBLISH(V,     "V",    currentData.V);
    PUBLISH(I,     "I",    currentData.I);
    PUBLISH(P,     "P",    currentData.P);
    PUBLISH(VPV,   "VPV",  currentData.VPV);
    PUBLISH(IPV,   "IPV",  currentData.IPV);
    PUBLISH(PPV,   "PPV",  currentData.PPV);
    PUBLISH(E,     "E",    currentData.E);
    PUBLISH(H19,   "H19",  currentData.H19);
    PUBLISH(H20,   "H20",  currentData.H20);
    PUBLISH(H21,   "H21",  currentData.H21);
    PUBLISH(H22,   "H22",  currentData.H22);
    PUBLISH(H23,   "H23",  currentData.H23);
#undef PUBLILSH
}

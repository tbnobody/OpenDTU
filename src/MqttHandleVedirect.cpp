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

            auto const& kvFrame = _kvFrames[optMpptData->serialNr_SER];
            publish_mppt_data(*optMpptData, kvFrame);
            if (!_PublishFull) {
                _kvFrames[optMpptData->serialNr_SER] = *optMpptData;
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
    topic.concat(currentData.serialNr_SER);
    topic.concat("/");

#define PUBLISH(sm, t, val) \
    if (_PublishFull || currentData.sm != previousData.sm) { \
        MqttSettings.publish(topic + t, String(val)); \
    }

    PUBLISH(productID_PID,           "PID",  currentData.getPidAsString().data());
    PUBLISH(serialNr_SER,            "SER",  currentData.serialNr_SER);
    PUBLISH(firmwareVer_FW,           "FW",  currentData.firmwareVer_FW);
    PUBLISH(loadOutputState_LOAD,   "LOAD",  (currentData.loadOutputState_LOAD ? "ON" : "OFF"));
    PUBLISH(currentState_CS,          "CS",  currentData.getCsAsString().data());
    PUBLISH(errorCode_ERR,           "ERR",  currentData.getErrAsString().data());
    PUBLISH(offReason_OR,             "OR",  currentData.getOrAsString().data());
    PUBLISH(stateOfTracker_MPPT,    "MPPT",  currentData.getMpptAsString().data());
    PUBLISH(daySequenceNr_HSDS,     "HSDS",  currentData.daySequenceNr_HSDS);
    PUBLISH(batteryVoltage_V_mV,       "V",  currentData.batteryVoltage_V_mV / 1000.0);
    PUBLISH(batteryCurrent_I_mA,       "I",  currentData.batteryCurrent_I_mA / 1000.0);
    PUBLISH(batteryOutputPower_W,      "P",  currentData.batteryOutputPower_W);
    PUBLISH(panelVoltage_VPV_mV,     "VPV",  currentData.panelVoltage_VPV_mV / 1000.0);
    PUBLISH(panelCurrent_mA,         "IPV",  currentData.panelCurrent_mA / 1000.0);
    PUBLISH(panelPower_PPV_W,        "PPV",  currentData.panelPower_PPV_W);
    PUBLISH(mpptEfficiency_Percent,    "E",  currentData.mpptEfficiency_Percent);
    PUBLISH(yieldTotal_H19_Wh,       "H19",  currentData.yieldTotal_H19_Wh / 1000.0);
    PUBLISH(yieldToday_H20_Wh,       "H20",  currentData.yieldToday_H20_Wh / 1000.0);
    PUBLISH(maxPowerToday_H21_W,     "H21",  currentData.maxPowerToday_H21_W);
    PUBLISH(yieldYesterday_H22_Wh,   "H22",  currentData.yieldYesterday_H22_Wh / 1000.0);
    PUBLISH(maxPowerYesterday_H23_W, "H23",  currentData.maxPowerYesterday_H23_W);
#undef PUBLILSH

#define PUBLISH_OPT(sm, t, val) \
    if (currentData.sm.first != 0 && (_PublishFull || currentData.sm.second != previousData.sm.second)) { \
        MqttSettings.publish(topic + t, String(val)); \
    }

    PUBLISH_OPT(NetworkTotalDcInputPowerMilliWatts,       "NetworkTotalDcInputPower",     currentData.NetworkTotalDcInputPowerMilliWatts.second / 1000.0);
    PUBLISH_OPT(MpptTemperatureMilliCelsius,              "MpptTemperature",              currentData.MpptTemperatureMilliCelsius.second / 1000.0);
    PUBLISH_OPT(BatteryAbsorptionMilliVolt,               "BatteryAbsorption",            currentData.BatteryAbsorptionMilliVolt.second / 1000.0);
    PUBLISH_OPT(BatteryFloatMilliVolt,                    "BatteryFloat",                 currentData.BatteryFloatMilliVolt.second / 1000.0);
    PUBLISH_OPT(SmartBatterySenseTemperatureMilliCelsius, "SmartBatterySenseTemperature", currentData.SmartBatterySenseTemperatureMilliCelsius.second / 1000.0);
#undef PUBLILSH_OPT
}

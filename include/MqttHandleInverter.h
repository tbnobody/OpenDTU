// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>
#include <espMqttClient.h>

class MqttHandleInverterClass {
public:
    MqttHandleInverterClass();
    void init(Scheduler& scheduler);

    static String getTopic(std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId);

    void subscribeTopics();
    void unsubscribeTopics();

private:
    void loop();
    void publishField(std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const FieldId_t fieldId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len, const size_t index, const size_t total);

    Task _loopTask;

    uint32_t _lastPublishStats[INV_MAX_COUNT] = { 0 };

    FieldId_t _publishFields[14] = {
        FLD_UDC,
        FLD_IDC,
        FLD_PDC,
        FLD_YD,
        FLD_YT,
        FLD_UAC,
        FLD_IAC,
        FLD_PAC,
        FLD_F,
        FLD_T,
        FLD_PF,
        FLD_EFF,
        FLD_IRR,
        FLD_Q
    };
};

extern MqttHandleInverterClass MqttHandleInverter;

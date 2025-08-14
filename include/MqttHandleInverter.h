// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Hoymiles.h>
#include <TaskSchedulerDeclarations.h>
#include <espMqttClient.h>
#include <frozen/map.h>
#include <frozen/string.h>

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

    enum class Topic : unsigned {
        LimitPersistentRelative,
        LimitPersistentAbsolute,
        LimitNonPersistentRelative,
        LimitNonPersistentAbsolute,
        Power,
        Restart,
        ResetRfStats,
    };

    static constexpr frozen::string _cmdtopic = "+/cmd/";
    static constexpr frozen::map<frozen::string, Topic, 7> _subscriptions = {
        { "limit_persistent_relative", Topic::LimitPersistentRelative },
        { "limit_persistent_absolute", Topic::LimitPersistentAbsolute },
        { "limit_nonpersistent_relative", Topic::LimitNonPersistentRelative },
        { "limit_nonpersistent_absolute", Topic::LimitNonPersistentAbsolute },
        { "power", Topic::Power },
        { "restart", Topic::Restart },
        { "reset_rf_stats", Topic::ResetRfStats },
    };

    void onMqttMessage(Topic t, const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len);
};

extern MqttHandleInverterClass MqttHandleInverter;

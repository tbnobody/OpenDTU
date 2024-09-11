// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <Huawei_can.h>
#include <espMqttClient.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>
#include <deque>
#include <functional>
#include <frozen/map.h>
#include <frozen/string.h>

class MqttHandleHuaweiClass {
public:
    void init(Scheduler& scheduler);

    void forceUpdate();

    void subscribeTopics();
    void unsubscribeTopics();

private:
    void loop();

    enum class Topic : unsigned {
        LimitOnlineVoltage,
        LimitOnlineCurrent,
        LimitOfflineVoltage,
        LimitOfflineCurrent,
        Mode
    };

    static constexpr frozen::string _cmdtopic = "huawei/cmd/";
    static constexpr frozen::map<frozen::string, Topic, 5> _subscriptions = {
        { "limit_online_voltage",  Topic::LimitOnlineVoltage },
        { "limit_online_current",  Topic::LimitOnlineCurrent },
        { "limit_offline_voltage", Topic::LimitOfflineVoltage },
        { "limit_offline_current", Topic::LimitOfflineCurrent },
        { "mode",                  Topic::Mode },
    };

    void onMqttMessage(Topic t,
            const espMqttClientTypes::MessageProperties& properties,
            const char* topic, const uint8_t* payload, size_t len,
            size_t index, size_t total);

    Task _loopTask;

    uint32_t _lastPublishStats;
    uint32_t _lastPublish;

    // MQTT callbacks to process updates on subscribed topics are executed in
    // the MQTT thread's context. we use this queue to switch processing the
    // user requests into the main loop's context (TaskScheduler context).
    mutable std::mutex _mqttMutex;
    std::deque<std::function<void()>> _mqttCallbacks;
};

extern MqttHandleHuaweiClass MqttHandleHuawei;
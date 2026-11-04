// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <cstdint>
#include <espMqttClient.h>
#include <frozen/map.h>
#include <frozen/string.h>

class MqttHandleDtuClass {
public:
    MqttHandleDtuClass();
    void init(Scheduler& scheduler);

    void subscribeTopics();
    void unsubscribeTopics();

private:
    void loop();
    void onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len);

    Task _loopTask;

    enum class Topic : unsigned {
        Restart,
    };

    static constexpr frozen::string _cmdtopic = "dtu/cmd/";
    static constexpr frozen::map<frozen::string, Topic, 1> _subscriptions = {
        { "restart", Topic::Restart },
    };
};

extern MqttHandleDtuClass MqttHandleDtu;

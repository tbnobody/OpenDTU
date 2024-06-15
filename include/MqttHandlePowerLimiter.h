// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>
#include <deque>
#include <functional>

class MqttHandlePowerLimiterClass {
public:
    void init(Scheduler& scheduler);

private:
    void loop();

    enum class MqttPowerLimiterCommand : unsigned {
        Mode,
        BatterySoCStartThreshold,
        BatterySoCStopThreshold,
        FullSolarPassthroughSoC,
        VoltageStartThreshold,
        VoltageStopThreshold,
        FullSolarPassThroughStartVoltage,
        FullSolarPassThroughStopVoltage,
        UpperPowerLimit
    };

    void onMqttCmd(MqttPowerLimiterCommand command, const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, size_t len, size_t index, size_t total);

    Task _loopTask;

    uint32_t _lastPublishStats;
    uint32_t _lastPublish;

    // MQTT callbacks to process updates on subscribed topics are executed in
    // the MQTT thread's context. we use this queue to switch processing the
    // user requests into the main loop's context (TaskScheduler context).
    mutable std::mutex _mqttMutex;
    std::deque<std::function<void()>> _mqttCallbacks;
};

extern MqttHandlePowerLimiterClass MqttHandlePowerLimiter;

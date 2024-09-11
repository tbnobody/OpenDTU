// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include <espMqttClient.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>
#include <deque>
#include <functional>
#include <frozen/map.h>
#include <frozen/string.h>

class MqttHandlePowerLimiterClass {
public:
    void init(Scheduler& scheduler);

    void forceUpdate();

    void subscribeTopics();
    void unsubscribeTopics();

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
        UpperPowerLimit,
        TargetPowerConsumption
    };

    static constexpr frozen::string _cmdtopic = "powerlimiter/cmd/";
    static constexpr frozen::map<frozen::string, MqttPowerLimiterCommand, 10> _subscriptions = {
        { "threshold/soc/start",                            MqttPowerLimiterCommand::BatterySoCStartThreshold },
        { "threshold/soc/stop",                             MqttPowerLimiterCommand::BatterySoCStopThreshold },
        { "threshold/soc/full_solar_passthrough",           MqttPowerLimiterCommand::FullSolarPassthroughSoC },
        { "threshold/voltage/start",                        MqttPowerLimiterCommand::VoltageStartThreshold },
        { "threshold/voltage/stop",                         MqttPowerLimiterCommand::VoltageStopThreshold },
        { "threshold/voltage/full_solar_passthrough_start", MqttPowerLimiterCommand::FullSolarPassThroughStartVoltage },
        { "threshold/voltage/full_solar_passthrough_stop",  MqttPowerLimiterCommand::FullSolarPassThroughStopVoltage },
        { "mode",                                           MqttPowerLimiterCommand::Mode },
        { "upper_power_limit",                              MqttPowerLimiterCommand::UpperPowerLimit },
        { "target_power_consumption",                       MqttPowerLimiterCommand::TargetPowerConsumption },
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

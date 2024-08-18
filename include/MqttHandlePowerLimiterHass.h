// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>

class MqttHandlePowerLimiterHassClass {
public:
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    void publish(const String& subtopic, const String& payload);
    void publishNumber(const char* caption, const char* icon, const char* category, const char* commandTopic, const char* stateTopic, const char* unitOfMeasure, const int16_t min, const int16_t max, const float step);
    void publishSelect(const char* caption, const char* icon, const char* category, const char* commandTopic, const char* stateTopic);
    void publishBinarySensor(const char* caption, const char* icon, const char* stateTopic, const char* payload_on, const char* payload_off);
    void createDeviceInfo(JsonDocument& root);

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
};

extern MqttHandlePowerLimiterHassClass MqttHandlePowerLimiterHass;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include <TaskSchedulerDeclarations.h>

class MqttHandlePylontechHassClass {
public:
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    void publish(const String& subtopic, const String& payload);
    void publishBinarySensor(const char* caption, const char* icon, const char* subTopic, const char* payload_on, const char* payload_off);
    void publishSensor(const char* caption, const char* icon, const char* subTopic, const char* deviceClass = NULL, const char* stateClass = NULL, const char* unitOfMeasurement = NULL);
    void createDeviceInfo(JsonObject& object);

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
    String serial = "0001"; // pseudo-serial, can be replaced in future with real serialnumber
};

extern MqttHandlePylontechHassClass MqttHandlePylontechHass;
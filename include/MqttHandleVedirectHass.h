// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ArduinoJson.h>
#include "VeDirectMpptController.h"
#include <TaskSchedulerDeclarations.h>

class MqttHandleVedirectHassClass {
public:
    void init(Scheduler& scheduler);
    void publishConfig();
    void forceUpdate();

private:
    void loop();
    void publish(const String& subtopic, const String& payload);
    void publishBinarySensor(const char *caption, const char *icon, const char *subTopic,
                             const char *payload_on, const char *payload_off,
                             const VeDirectMpptController::data_t &mpptData);
    void publishSensor(const char *caption, const char *icon, const char *subTopic,
                       const char *deviceClass, const char *stateClass,
                       const char *unitOfMeasurement,
                       const VeDirectMpptController::data_t &mpptData);
    void createDeviceInfo(JsonObject &object,
                          const VeDirectMpptController::data_t &mpptData);

    Task _loopTask;

    bool _wasConnected = false;
    bool _updateForced = false;
};

extern MqttHandleVedirectHassClass MqttHandleVedirectHass;

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "MqttHandleDtu.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>

MqttHandleDtuClass MqttHandleDtu;

void MqttHandleDtuClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleDtuClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleDtuClass::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    MqttSettings.publish("dtu/uptime", String(millis() / 1000));
    MqttSettings.publish("dtu/ip", NetworkSettings.localIP().toString());
    MqttSettings.publish("dtu/hostname", NetworkSettings.getHostname());
    if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
        MqttSettings.publish("dtu/rssi", String(WiFi.RSSI()));
        MqttSettings.publish("dtu/bssid", WiFi.BSSIDstr());
    }
}
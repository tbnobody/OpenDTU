// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "MqttHandleInverterTotal.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include <Hoymiles.h>

MqttHandleInverterTotalClass MqttHandleInverterTotal;

MqttHandleInverterTotalClass::MqttHandleInverterTotalClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleInverterTotalClass::loop, this))
{
}

void MqttHandleInverterTotalClass::init(Scheduler& scheduler)
{
    this->_connected_iterations = 0;
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleInverterTotalClass::loop()
{
    // Update interval from config
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected()) {
        this->_connected_iterations = 0;
    } else {
        this->_connected_iterations++;
    }

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    if(this->_connected_iterations < 2) {
        // publish is_valid false after connecting to ensure statistics don't get a wrong value during startup
        MqttSettings.publish("ac/is_valid", String(false));
        MqttSettings.publish("dc/is_valid", String(false));
        return;
    }

    MqttSettings.publish("ac/power", String(Datastore.getTotalAcPowerEnabled(), Datastore.getTotalAcPowerDigits()));
    MqttSettings.publish("ac/yieldtotal", String(Datastore.getTotalAcYieldTotalEnabled(), Datastore.getTotalAcYieldTotalDigits()));
    MqttSettings.publish("ac/yieldday", String(Datastore.getTotalAcYieldDayEnabled(), Datastore.getTotalAcYieldDayDigits()));
    MqttSettings.publish("ac/is_valid", String(Datastore.getIsAllEnabledReachable()));
    MqttSettings.publish("dc/power", String(Datastore.getTotalDcPowerEnabled(), Datastore.getTotalDcPowerDigits()));
    MqttSettings.publish("dc/irradiation", String(Datastore.getTotalDcIrradiation(), 3));
    MqttSettings.publish("dc/is_valid", String(Datastore.getIsAllEnabledReachable()));
}

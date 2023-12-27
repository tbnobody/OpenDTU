// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "MqttHandleInverterTotal.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include <Hoymiles.h>

MqttHandleInverterTotalClass MqttHandleInverterTotal;

void MqttHandleInverterTotalClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleInverterTotalClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleInverterTotalClass::loop()
{
    // Update interval from config
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
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

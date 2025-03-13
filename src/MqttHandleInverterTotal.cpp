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
    _availableHandler.reset(new MqttAvailableHandler(std::bind(&MqttHandleInverterTotalClass::sendData, this)));
}

void MqttHandleInverterTotalClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

bool MqttHandleInverterTotalClass::isDataValid() {
    return Datastore.getIsAllEnabledReachable() && Datastore.getIsAtLeastOneReachable();
}

void MqttHandleInverterTotalClass::sendData() {
    MqttSettings.publish("ac/power", String(Datastore.getTotalAcPowerEnabled(), Datastore.getTotalAcPowerDigits()));
    MqttSettings.publish("ac/yieldtotal", String(Datastore.getTotalAcYieldTotalEnabled(), Datastore.getTotalAcYieldTotalDigits()));
    MqttSettings.publish("ac/yieldday", String(Datastore.getTotalAcYieldDayEnabled(), Datastore.getTotalAcYieldDayDigits()));
    MqttSettings.publish("dc/power", String(Datastore.getTotalDcPowerEnabled(), Datastore.getTotalDcPowerDigits()));
    MqttSettings.publish("dc/irradiation", String(Datastore.getTotalDcIrradiation(), 3));
    MqttSettings.publish("dc/is_valid", String(this->isDataValid()));
}

void MqttHandleInverterTotalClass::loop()
{
    // Update interval from config
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    // publishes is_valid and calls sendData when appropriate
    _availableHandler->send("ac/is_valid", this->isDataValid());

}

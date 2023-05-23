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

void MqttHandleInverterTotalClass::init()
{
    _lastPublish.set(Configuration.get().Mqtt_PublishInterval * 1000);
}

void MqttHandleInverterTotalClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        return;
    }

    if (_lastPublish.occured()) {
        MqttSettings.publish("ac/power", String(Datastore.totalAcPowerEnabled, Datastore.totalAcPowerDigits));
        MqttSettings.publish("ac/yieldtotal", String(Datastore.totalAcYieldTotalEnabled, Datastore.totalAcYieldTotalDigits));
        MqttSettings.publish("ac/yieldday", String(Datastore.totalAcYieldDayEnabled, Datastore.totalAcYieldDayDigits));
        MqttSettings.publish("ac/is_valid", String(Datastore.isAllEnabledReachable));
        MqttSettings.publish("dc/power", String(Datastore.totalDcPowerEnabled, Datastore.totalDcPowerDigits));
        MqttSettings.publish("dc/irradiation", String(Datastore.totalDcIrradiation, 3));
        MqttSettings.publish("dc/is_valid", String(Datastore.isAllEnabledReachable));

        _lastPublish.set(Configuration.get().Mqtt_PublishInterval * 1000);
    }
}

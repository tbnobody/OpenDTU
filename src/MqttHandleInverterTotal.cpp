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
        MqttSettings.publish("ac/power", String(Datastore.getTotalAcPowerEnabled(), Datastore.getTotalAcPowerDigits()));
        MqttSettings.publish("ac/yieldtotal", String(Datastore.getTotalAcYieldTotalEnabled(), Datastore.getTotalAcYieldTotalDigits()));
        MqttSettings.publish("ac/yieldday", String(Datastore.getTotalAcYieldDayEnabled(), Datastore.getTotalAcYieldDayDigits()));
        MqttSettings.publish("ac/is_valid", String(Datastore.getIsAllEnabledReachable()));
        MqttSettings.publish("dc/power", String(Datastore.getTotalDcPowerEnabled(), Datastore.getTotalDcPowerDigits()));
        MqttSettings.publish("dc/irradiation", String(Datastore.getTotalDcIrradiation(), 3));
        MqttSettings.publish("dc/is_valid", String(Datastore.getIsAllEnabledReachable()));

        _lastPublish.set(Configuration.get().Mqtt_PublishInterval * 1000);
    }
}

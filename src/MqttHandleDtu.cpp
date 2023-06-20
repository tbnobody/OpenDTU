// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHandleDtu.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include <Hoymiles.h>

MqttHandleDtuClass MqttHandleDtu;

void MqttHandleDtuClass::init()
{
}

void MqttHandleDtuClass::loop()
{
    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    if (millis() - _lastPublish > (config.Mqtt_PublishInterval * 1000)) {
        MqttSettings.publish("dtu/uptime", String(millis() / 1000));
        MqttSettings.publish("dtu/ip", NetworkSettings.localIP().toString());
        MqttSettings.publish("dtu/hostname", NetworkSettings.getHostname());
        if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
            MqttSettings.publish("dtu/rssi", String(WiFi.RSSI()));
            MqttSettings.publish("dtu/bssid", String(WiFi.BSSIDstr()));
        }

        _lastPublish = millis();
    }
}
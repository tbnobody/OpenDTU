// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHassPublishing.h"
#include "ArduinoJson.h"
#include "MqttPublishing.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"

MqttHassPublishingClass MqttHassPublishing;

void MqttHassPublishingClass::init()
{
}

void MqttHassPublishingClass::loop()
{
    if (_updateForced) {
        publishConfig();
        _updateForced = false;
    }

    if (MqttSettings.getConnected() && !_wasConnected) {
        // Connection established
        _wasConnected = true;
        publishConfig();
    } else if (!MqttSettings.getConnected() && _wasConnected) {
        // Connection lost
        _wasConnected = false;
    }
}

void MqttHassPublishingClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHassPublishingClass::publishConfig()
{
    if (!Configuration.get().Mqtt_Hass_Enabled) {
        return;
    }

    if (!MqttSettings.getConnected() && Hoymiles.getRadio()->isIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        // Loop all channels
        for (uint8_t c = 0; c <= inv->Statistics()->getChannelCount(); c++) {
            for (uint8_t f = 0; f < DEVICE_CLS_ASSIGN_LIST_LEN; f++) {
                bool clear = false;
                if (c > 0 && !config.Mqtt_Hass_IndividualPanels) {
                    clear = true;
                }
                publishField(inv, c, deviceFieldAssignment[f], clear);
            }
        }

        yield();
    }
}

void MqttHassPublishingClass::publishField(std::shared_ptr<InverterAbstract> inv, uint8_t channel, byteAssign_fieldDeviceClass_t fieldType, bool clear)
{
    if (!inv->Statistics()->hasChannelFieldValue(channel, fieldType.fieldId)) {
        return;
    }

    char serial[sizeof(uint64_t) * 8 + 1];
    snprintf(serial, sizeof(serial), "%0lx%08lx",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

    String fieldName;
    if (channel == CH0 && fieldType.fieldId == FLD_PDC) {
        fieldName = "PowerDC";
    } else {
        fieldName = inv->Statistics()->getChannelFieldName(channel, fieldType.fieldId);
    }

    String configTopic = "sensor/dtu_" + String(serial)
        + "/" + "ch" + String(channel) + "_" + fieldName
        + "/config";

    if (!clear) {
        String stateTopic = MqttSettings.getPrefix() + MqttPublishing.getTopic(inv, channel, fieldType.fieldId);
        const char* devCls = deviceClasses[fieldType.deviceClsId];
        const char* stateCls = stateClasses[fieldType.stateClsId];

        String name;
        if (channel == CH0) {
            name = String(inv->name()) + " " + fieldName;
        } else {
            name = String(inv->name()) + " CH" + String(channel) + " " + fieldName;
        }

        DynamicJsonDocument deviceDoc(512);
        deviceDoc[F("name")] = inv->name();
        deviceDoc[F("ids")] = String(serial);
        deviceDoc[F("cu")] = String(F("http://")) + String(WiFi.localIP().toString());
        deviceDoc[F("mf")] = F("OpenDTU");
        deviceDoc[F("mdl")] = inv->typeName();
        deviceDoc[F("sw")] = AUTO_GIT_HASH;
        JsonObject deviceObj = deviceDoc.as<JsonObject>();

        DynamicJsonDocument root(1024);
        root[F("name")] = name;
        root[F("stat_t")] = stateTopic;
        root[F("unit_of_meas")] = inv->Statistics()->getChannelFieldUnit(channel, fieldType.fieldId);
        root[F("uniq_id")] = String(serial) + "_ch" + String(channel) + "_" + fieldName;
        root[F("dev")] = deviceObj;
        if (Configuration.get().Mqtt_Hass_Expire) {
            root[F("exp_aft")] = Hoymiles.getNumInverters() * Configuration.get().Mqtt_PublishInterval * 2;
        }
        if (devCls != 0) {
            root[F("dev_cla")] = devCls;
        }
        if (stateCls != 0) {
            root[F("stat_cla")] = stateCls;
        }

        char buffer[512];
        serializeJson(root, buffer);
        MqttSettings.publishHass(configTopic, buffer);
    }
    else {
        MqttSettings.publishHass(configTopic, "");
    }
}
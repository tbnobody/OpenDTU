// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHassPublishing.h"
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

        publishInverterButton(inv, "Turn Inverter Off", "mdi:power-plug-off", "config", "", "cmd/power", "0");
        publishInverterButton(inv, "Turn Inverter On", "mdi:power-plug", "config", "", "cmd/power", "1");
        publishInverterButton(inv, "Restart Inverter", "", "config", "restart", "cmd/restart", "1");

        publishInverterNumber(inv, "Limit NonPersistent Relative", "mdi:speedometer", "config", "cmd/limit_nonpersistent_relative", "status/limit_relative", "%");
        publishInverterNumber(inv, "Limit Persistent Relative", "mdi:speedometer", "config", "cmd/limit_persistent_relative", "status/limit_relative", "%");

        publishInverterNumber(inv, "Limit NonPersistent Absolute", "mdi:speedometer", "config", "cmd/limit_nonpersistent_absolute", "status/limit_absolute", "W", 10, 1500);
        publishInverterNumber(inv, "Limit Persistent Absolute", "mdi:speedometer", "config", "cmd/limit_persistent_absolute", "status/limit_absolute", "W", 10, 1500);

        publishInverterBinarySensor(inv, "Reachable", "status/reachable", "1", "0");
        publishInverterBinarySensor(inv, "Producing", "status/producing", "1", "0");

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
    snprintf(serial, sizeof(serial), "%0x%08x",
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

        DynamicJsonDocument root(1024);
        root[F("name")] = name;
        root[F("stat_t")] = stateTopic;
        root[F("unit_of_meas")] = inv->Statistics()->getChannelFieldUnit(channel, fieldType.fieldId);
        root[F("uniq_id")] = String(serial) + "_ch" + String(channel) + "_" + fieldName;

        JsonObject deviceObj = root.createNestedObject("dev");
        createDeviceInfo(deviceObj, inv);

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
    } else {
        MqttSettings.publishHass(configTopic, "");
    }
}

void MqttHassPublishingClass::publishInverterButton(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category, const char* deviceClass, const char* subTopic, const char* payload)
{
    char serial[sizeof(uint64_t) * 8 + 1];
    snprintf(serial, sizeof(serial), "%0x%08x",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

    String buttonId = caption;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    String configTopic = "button/dtu_" + String(serial)
        + "/" + buttonId
        + "/config";

    String cmdTopic = MqttSettings.getPrefix() + String(serial) + "/" + subTopic;

    DynamicJsonDocument root(1024);
    root[F("name")] = caption;
    root[F("uniq_id")] = String(serial) + "_" + buttonId;
    if (strcmp(icon, "")) {
        root[F("ic")] = icon;
    }
    if (strcmp(deviceClass, "")) {
        root[F("dev_cla")] = deviceClass;
    }
    root[F("ent_cat")] = category;
    root[F("cmd_t")] = cmdTopic;
    root[F("payload_press")] = payload;

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj, inv);

    char buffer[512];
    serializeJson(root, buffer);
    MqttSettings.publishHass(configTopic, buffer);
}

void MqttHassPublishingClass::publishInverterNumber(
    std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category,
    const char* commandTopic, const char* stateTopic, const char* unitOfMeasure,
    int16_t min, int16_t max)
{
    char serial[sizeof(uint64_t) * 8 + 1];
    snprintf(serial, sizeof(serial), "%0x%08x",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

    String buttonId = caption;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    String configTopic = "number/dtu_" + String(serial)
        + "/" + buttonId
        + "/config";

    String cmdTopic = MqttSettings.getPrefix() + String(serial) + "/" + commandTopic;
    String statTopic = MqttSettings.getPrefix() + String(serial) + "/" + stateTopic;

    DynamicJsonDocument root(1024);
    root[F("name")] = caption;
    root[F("uniq_id")] = String(serial) + "_" + buttonId;
    if (strcmp(icon, "")) {
        root[F("ic")] = icon;
    }
    root[F("ent_cat")] = category;
    root[F("cmd_t")] = cmdTopic;
    root[F("stat_t")] = statTopic;
    root[F("unit_of_meas")] = unitOfMeasure;
    root[F("min")] = min;
    root[F("max")] = max;

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj, inv);

    char buffer[512];
    serializeJson(root, buffer);
    MqttSettings.publishHass(configTopic, buffer);
}

void MqttHassPublishingClass::publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* subTopic, const char* payload_on, const char* payload_off)
{
    char serial[sizeof(uint64_t) * 8 + 1];
    snprintf(serial, sizeof(serial), "%0x%08x",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.toLowerCase();

    String configTopic = "binary_sensor/dtu_" + String(serial)
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + String(serial) + "/" + subTopic;

    DynamicJsonDocument root(1024);
    root[F("name")] = caption;
    root[F("uniq_id")] = String(serial) + "_" + sensorId;
    root[F("stat_t")] = statTopic;
    root[F("pl_on")] = payload_on;
    root[F("pl_off")] = payload_off;

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj, inv);

    char buffer[512];
    serializeJson(root, buffer);
    MqttSettings.publishHass(configTopic, buffer);
}

void MqttHassPublishingClass::createDeviceInfo(JsonObject& object, std::shared_ptr<InverterAbstract> inv)
{
    char serial[sizeof(uint64_t) * 8 + 1];
    snprintf(serial, sizeof(serial), "%0x%08x",
        ((uint32_t)((inv->serial() >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(inv->serial() & 0xFFFFFFFF)));

    object[F("name")] = inv->name();
    object[F("ids")] = String(serial);
    object[F("cu")] = String(F("http://")) + WiFi.localIP().toString();
    object[F("mf")] = F("OpenDTU");
    object[F("mdl")] = inv->typeName();
    object[F("sw")] = AUTO_GIT_HASH;
}
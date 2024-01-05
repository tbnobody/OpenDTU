// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHandleVedirectHass.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "MessageOutput.h"
#include "VictronMppt.h"

MqttHandleVedirectHassClass MqttHandleVedirectHass;

void MqttHandleVedirectHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleVedirectHassClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
}

void MqttHandleVedirectHassClass::loop()
{
    if (!Configuration.get().Vedirect.Enabled) {
        return;
    }
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

void MqttHandleVedirectHassClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHandleVedirectHassClass::publishConfig()
{
    if ((!Configuration.get().Mqtt.Hass.Enabled) ||
       (!Configuration.get().Vedirect.Enabled)) {
        return;
    }

    if (!MqttSettings.getConnected()) {
        return;
    }
    // ensure data is revieved from victron
    if (!VictronMppt.isDataValid()) {
        return;
    }

    // device info
    publishBinarySensor("MPPT load output state", "mdi:export", "LOAD", "ON", "OFF");
    publishSensor("MPPT serial number", "mdi:counter", "SER");
    publishSensor("MPPT firmware number", "mdi:counter", "FW");
    publishSensor("MPPT state of operation", "mdi:wrench", "CS");
    publishSensor("MPPT error code", "mdi:bell", "ERR");
    publishSensor("MPPT off reason", "mdi:wrench", "OR");
    publishSensor("MPPT tracker operation mode", "mdi:wrench", "MPPT");
    publishSensor("MPPT Day sequence number (0...364)", "mdi:calendar-month-outline", "HSDS", NULL, "total", "d");

    // battery info
    publishSensor("Battery voltage", NULL, "V", "voltage", "measurement", "V");
    publishSensor("Battery current", NULL, "I", "current", "measurement", "A");
    publishSensor("Battery power (calculated)", NULL, "P", "power", "measurement", "W");
    publishSensor("Battery efficiency (calculated)", NULL, "E", NULL, "measurement", "%");
    
    // panel info
    publishSensor("Panel voltage", NULL, "VPV", "voltage", "measurement", "V");
    publishSensor("Panel current (calculated)", NULL, "IPV", "current", "measurement", "A");
    publishSensor("Panel power", NULL, "PPV", "power", "measurement", "W");
    publishSensor("Panel yield total", NULL, "H19", "energy", "total_increasing", "kWh");
    publishSensor("Panel yield today", NULL, "H20", "energy", "total", "kWh");
    publishSensor("Panel maximum power today", NULL, "H21", "power", "measurement", "W");
    publishSensor("Panel yield yesterday", NULL, "H22", "energy", "total", "kWh");
    publishSensor("Panel maximum power yesterday", NULL, "H23", "power", "measurement", "W");

    yield();
}

void MqttHandleVedirectHassClass::publishSensor(const char* caption, const char* icon, const char* subTopic, const char* deviceClass, const char* stateClass, const char* unitOfMeasurement )
{
    String serial = VictronMppt.getData()->SER;

    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.toLowerCase();

    String configTopic = "sensor/dtu_victron_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "victron/";
    statTopic.concat(serial);
    statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    root["name"] = caption;
    root["stat_t"] = statTopic;
    root["uniq_id"] = serial + "_" + sensorId;

    if (icon != NULL) {
        root["icon"] = icon;
    }

    if (unitOfMeasurement != NULL) {
        root["unit_of_meas"] = unitOfMeasurement;
    }

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    if (Configuration.get().Mqtt.Hass.Expire) {
        root["exp_aft"] = Configuration.get().Mqtt.PublishInterval * 3;
    }
    if (deviceClass != NULL) {
        root["dev_cla"] = deviceClass;
    }
    if (stateClass != NULL) {
        root["stat_cla"] = stateClass;
    }

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);

}
void MqttHandleVedirectHassClass::publishBinarySensor(const char* caption, const char* icon, const char* subTopic, const char* payload_on, const char* payload_off)
{
    String serial = VictronMppt.getData()->SER;

    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.replace(".", "");
    sensorId.replace("(", "");
    sensorId.replace(")", "");
    sensorId.toLowerCase();

    String configTopic = "binary_sensor/dtu_victron_" + serial
        + "/" + sensorId
        + "/config";

    String statTopic = MqttSettings.getPrefix() + "victron/";
    statTopic.concat(serial);
    statTopic.concat("/");
    statTopic.concat(subTopic);

    DynamicJsonDocument root(1024);
    root["name"] = caption;
    root["uniq_id"] = serial + "_" + sensorId;
    root["stat_t"] = statTopic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    if (icon != NULL) {
        root["icon"] = icon;
    }

    JsonObject deviceObj = root.createNestedObject("dev");
    createDeviceInfo(deviceObj);

    char buffer[512];
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleVedirectHassClass::createDeviceInfo(JsonObject& object)
{
    auto spMpptData = VictronMppt.getData();
    String serial = spMpptData->SER;
    object["name"] = "Victron(" + serial + ")";
    object["ids"] = serial;
    object["cu"] = String("http://") + NetworkSettings.localIP().toString();
    object["mf"] = "OpenDTU";
    object["mdl"] = spMpptData->getPidAsString();
    object["sw"] = AUTO_GIT_HASH;
}

void MqttHandleVedirectHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt.Hass.Retain);
}

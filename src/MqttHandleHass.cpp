// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "defaults.h"

MqttHandleHassClass MqttHandleHass;

void MqttHandleHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandleHassClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
}

void MqttHandleHassClass::loop()
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

void MqttHandleHassClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHandleHassClass::publishConfig()
{
    if (!Configuration.get().Mqtt.Hass.Enabled) {
        return;
    }

    if (!MqttSettings.getConnected() && Hoymiles.isAllRadioIdle()) {
        return;
    }

    const CONFIG_T& config = Configuration.get();

     // publish DTU sensors
    publishDtuSensor("IP", "", "diagnostic", "mdi:network-outline", "", "");
    publishDtuSensor("WiFi Signal", "signal_strength", "diagnostic", "", "dBm", "rssi");
    publishDtuSensor("Uptime", "duration", "diagnostic", "", "s", "");
    publishDtuBinarySensor("Status", "connectivity", "diagnostic", config.Mqtt.Lwt.Value_Online, config.Mqtt.Lwt.Value_Offline, config.Mqtt.Lwt.Topic);

    yield();

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        publishInverterButton(inv, "Turn Inverter Off", "mdi:power-plug-off", "config", "", "cmd/power", "0");
        publishInverterButton(inv, "Turn Inverter On", "mdi:power-plug", "config", "", "cmd/power", "1");
        publishInverterButton(inv, "Restart Inverter", "", "config", "restart", "cmd/restart", "1");

        publishInverterNumber(inv, "Limit NonPersistent Relative", "mdi:speedometer", "config", "cmd/limit_nonpersistent_relative", "status/limit_relative", "%", 0, 100);
        publishInverterNumber(inv, "Limit Persistent Relative", "mdi:speedometer", "config", "cmd/limit_persistent_relative", "status/limit_relative", "%", 0, 100);

        publishInverterNumber(inv, "Limit NonPersistent Absolute", "mdi:speedometer", "config", "cmd/limit_nonpersistent_absolute", "status/limit_absolute", "W", 0, MAX_INVERTER_LIMIT);
        publishInverterNumber(inv, "Limit Persistent Absolute", "mdi:speedometer", "config", "cmd/limit_persistent_absolute", "status/limit_absolute", "W", 0, MAX_INVERTER_LIMIT);

        publishInverterBinarySensor(inv, "Reachable", "status/reachable", "1", "0");
        publishInverterBinarySensor(inv, "Producing", "status/producing", "1", "0");

        // Loop all channels
        for (auto& t : inv->Statistics()->getChannelTypes()) {
            for (auto& c : inv->Statistics()->getChannelsByType(t)) {
                for (uint8_t f = 0; f < DEVICE_CLS_ASSIGN_LIST_LEN; f++) {
                    bool clear = false;
                    if (t == TYPE_DC && !config.Mqtt.Hass.IndividualPanels) {
                        clear = true;
                    }
                    publishInverterField(inv, t, c, deviceFieldAssignment[f], clear);
                }
            }
        }

        yield();
    }
}

void MqttHandleHassClass::publishInverterField(std::shared_ptr<InverterAbstract> inv, const ChannelType_t type, const ChannelNum_t channel, const byteAssign_fieldDeviceClass_t fieldType, const bool clear)
{
    if (!inv->Statistics()->hasChannelFieldValue(type, channel, fieldType.fieldId)) {
        return;
    }

    const String serial = inv->serialString();

    String fieldName;
    if (type == TYPE_AC && fieldType.fieldId == FLD_PDC) {
        fieldName = "PowerDC";
    } else {
        fieldName = inv->Statistics()->getChannelFieldName(type, channel, fieldType.fieldId);
    }

    String chanNum;
    if (type == TYPE_DC) {
        // TODO(tbnobody)
        chanNum = static_cast<uint8_t>(channel) + 1;
    } else {
        chanNum = channel;
    }

    const String configTopic = "sensor/dtu_" + serial
        + "/" + "ch" + chanNum + "_" + fieldName
        + "/config";

    if (!clear) {
        const String stateTopic = MqttSettings.getPrefix() + MqttHandleInverter.getTopic(inv, type, channel, fieldType.fieldId);
        const char* devCls = deviceClasses[fieldType.deviceClsId];
        const char* stateCls = stateClasses[fieldType.stateClsId];

        String name;
        if (type != TYPE_DC) {
            name = fieldName;
        } else {
            name = "CH" + chanNum + " " + fieldName;
        }

        DynamicJsonDocument root(1024);
        root["name"] = name;
        root["stat_t"] = stateTopic;
        root["uniq_id"] = serial + "_ch" + chanNum + "_" + fieldName;

        String unit_of_measure = inv->Statistics()->getChannelFieldUnit(type, channel, fieldType.fieldId);
        if (unit_of_measure != "") {
            root["unit_of_meas"] = unit_of_measure;
        }

        createInverterInfo(root, inv);

        if (Configuration.get().Mqtt.Hass.Expire) {
            root["exp_aft"] = Hoymiles.getNumInverters() * max<uint32_t>(Hoymiles.PollInterval(), Configuration.get().Mqtt.PublishInterval) * inv->getReachableThreshold();
        }
        if (devCls != 0) {
            root["dev_cla"] = devCls;
        }
        if (stateCls != 0) {
            root["stat_cla"] = stateCls;
        }

        String buffer;
        serializeJson(root, buffer);
        publish(configTopic, buffer);
    } else {
        publish(configTopic, "");
    }
}

void MqttHandleHassClass::publishInverterButton(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category, const char* deviceClass, const char* subTopic, const char* payload)
{
    const String serial = inv->serialString();

    String buttonId = caption;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "button/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + subTopic;

    DynamicJsonDocument root(1024);
    root["name"] = caption;
    root["uniq_id"] = serial + "_" + buttonId;
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    if (strcmp(deviceClass, "")) {
        root["dev_cla"] = deviceClass;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["payload_press"] = payload;

    createInverterInfo(root, inv);

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishInverterNumber(
    std::shared_ptr<InverterAbstract> inv, const char* caption, const char* icon, const char* category,
    const char* commandTopic, const char* stateTopic, const char* unitOfMeasure,
    const int16_t min, const int16_t max)
{
    const String serial = inv->serialString();

    String buttonId = caption;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "number/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + commandTopic;
    const String statTopic = MqttSettings.getPrefix() + serial + "/" + stateTopic;

    DynamicJsonDocument root(1024);
    root["name"] = caption;
    root["uniq_id"] = serial + "_" + buttonId;
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["stat_t"] = statTopic;
    root["unit_of_meas"] = unitOfMeasure;
    root["min"] = min;
    root["max"] = max;

    createInverterInfo(root, inv);

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const char* caption, const char* subTopic, const char* payload_on, const char* payload_off)
{
    const String serial = inv->serialString();

    String sensorId = caption;
    sensorId.replace(" ", "_");
    sensorId.toLowerCase();

    const String configTopic = "binary_sensor/dtu_" + serial
        + "/" + sensorId
        + "/config";

    const String statTopic = MqttSettings.getPrefix() + serial + "/" + subTopic;

    DynamicJsonDocument root(1024);
    root["name"] = caption;
    root["uniq_id"] = serial + "_" + sensorId;
    root["stat_t"] = statTopic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    createInverterInfo(root, inv);

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishDtuSensor(const char* name, const char* device_class, const char* category, const char* icon, const char* unit_of_measure, const char* subTopic)
{
    String id = name;
    id.toLowerCase();
    id.replace(" ", "_");
    String topic = subTopic;
    if (topic == "") {
        topic = id;
    }

    DynamicJsonDocument root(1024);
    root["name"] = name;
    root["uniq_id"] = getDtuUniqueId() + "_" + id;
    if (strcmp(device_class, "")) {
        root["dev_cla"] = device_class;
    }
    if (strcmp(category, "")) {
        root["ent_cat"] = category;
    }
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    if (strcmp(unit_of_measure, "")) {
        root["unit_of_meas"] = unit_of_measure;
    }
    root["stat_t"] = MqttSettings.getPrefix() + "dtu" + "/" + topic;

    root["avty_t"] = MqttSettings.getPrefix() + Configuration.get().Mqtt.Lwt.Topic;

    const CONFIG_T& config = Configuration.get();
    root["pl_avail"] = config.Mqtt.Lwt.Value_Online;
    root["pl_not_avail"] = config.Mqtt.Lwt.Value_Offline;

    createDtuInfo(root);

    String buffer;
    const String configTopic = "sensor/" + getDtuUniqueId() + "/" + id + "/config";
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishDtuBinarySensor(const char* name, const char* device_class, const char* category, const char* payload_on, const char* payload_off, const char* subTopic)
{
    String id = name;
    id.toLowerCase();
    id.replace(" ", "_");

    String topic = subTopic;
    if (!strcmp(subTopic, "")) {
        topic = String("dtu/") + "/" + id;
    }

    DynamicJsonDocument root(1024);
    root["name"] = name;
    root["uniq_id"] = getDtuUniqueId() + "_" + id;
    root["stat_t"] = MqttSettings.getPrefix() + topic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    if (strcmp(device_class, "")) {
        root["dev_cla"] = device_class;
    }
    if (strcmp(category, "")) {
        root["ent_cat"] = category;
    }

    createDtuInfo(root);

    String buffer;
    const String configTopic = "binary_sensor/" + getDtuUniqueId() + "/" + id + "/config";
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::createInverterInfo(DynamicJsonDocument& root, std::shared_ptr<InverterAbstract> inv)
{
    createDeviceInfo(
        root,
        inv->name(),
        inv->serialString(),
        getDtuUrl(),
        "OpenDTU",
        inv->typeName(),
        AUTO_GIT_HASH,
        getDtuUniqueId());
}

void MqttHandleHassClass::createDtuInfo(DynamicJsonDocument& root)
{
    createDeviceInfo(
        root,
        NetworkSettings.getHostname(),
        getDtuUniqueId(),
        getDtuUrl(),
        "OpenDTU",
        "OpenDTU",
        AUTO_GIT_HASH);
}

void MqttHandleHassClass::createDeviceInfo(
    DynamicJsonDocument& root,
    const String& name, const String& identifiers, const String& configuration_url,
    const String& manufacturer, const String& model, const String& sw_version,
    const String& via_device)
{
    auto object = root.createNestedObject("dev");

    object["name"] = name;
    object["ids"] = identifiers;
    object["cu"] = configuration_url;
    object["mf"] = manufacturer;
    object["mdl"] = model;
    object["sw"] = sw_version;

    if (via_device != "") {
        object["via_device"] = via_device;
    }
}

String MqttHandleHassClass::getDtuUniqueId()
{
    return NetworkSettings.getHostname() + "_" + Utils::getChipId();
}

String MqttHandleHassClass::getDtuUrl()
{
    return String("http://") + NetworkSettings.localIP().toString();
}

void MqttHandleHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt.Hass.Retain);
}
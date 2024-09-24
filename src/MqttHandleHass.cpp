// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "defaults.h"
#include "__compiled_constants.h"

MqttHandleHassClass MqttHandleHass;

MqttHandleHassClass::MqttHandleHassClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleHassClass::loop, this))
{
}

void MqttHandleHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
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
    publishDtuSensor("IP", "ip", "", "mdi:network-outline", "", "diagnostic");
    publishDtuSensor("WiFi Signal", "rssi", "dBm", "", "signal_strength", "diagnostic");
    publishDtuSensor("Uptime", "uptime", "s", "", "duration", "diagnostic");
    publishDtuSensor("Temperature", "temperature", "°C", "mdi:thermometer", "temperature", "diagnostic");
    publishDtuSensor("Heap Size", "heap/size", "Bytes", "mdi:memory", "", "diagnostic");
    publishDtuSensor("Heap Free", "heap/free", "Bytes", "mdi:memory", "", "diagnostic");
    publishDtuSensor("Largest Free Heap Block", "heap/maxalloc", "Bytes", "mdi:memory", "", "diagnostic");
    publishDtuSensor("Lifetime Minimum Free Heap", "heap/minfree", "Bytes", "mdi:memory", "", "diagnostic");

    publishDtuBinarySensor("Status", config.Mqtt.Lwt.Topic, config.Mqtt.Lwt.Value_Online, config.Mqtt.Lwt.Value_Offline, "connectivity", "diagnostic");

    yield();

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        publishInverterButton(inv, "Turn Inverter Off", "mdi:power-plug-off", "config", "", "cmd/power", "0");
        publishInverterButton(inv, "Turn Inverter On", "mdi:power-plug", "config", "", "cmd/power", "1");
        publishInverterButton(inv, "Restart Inverter", "", "config", "restart", "cmd/restart", "1");
        publishInverterButton(inv, "Reset Radio Statistics", "", "config", "", "cmd/reset_rf_stats", "1");

        publishInverterNumber(inv, "Limit NonPersistent Relative", "mdi:speedometer", "config", "cmd/limit_nonpersistent_relative", "status/limit_relative", "%", 0, 100, 0.1);
        publishInverterNumber(inv, "Limit Persistent Relative", "mdi:speedometer", "config", "cmd/limit_persistent_relative", "status/limit_relative", "%", 0, 100, 0.1);

        publishInverterNumber(inv, "Limit NonPersistent Absolute", "mdi:speedometer", "config", "cmd/limit_nonpersistent_absolute", "status/limit_absolute", "W", 0, MAX_INVERTER_LIMIT);
        publishInverterNumber(inv, "Limit Persistent Absolute", "mdi:speedometer", "config", "cmd/limit_persistent_absolute", "status/limit_absolute", "W", 0, MAX_INVERTER_LIMIT);

        publishInverterBinarySensor(inv, "Reachable", "status/reachable", "1", "0");
        publishInverterBinarySensor(inv, "Producing", "status/producing", "1", "0");

        yield();

        publishInverterSensor(inv, "TX Requests", "radio/tx_request", "", "", "", "diagnostic");
        publishInverterSensor(inv, "RX Success", "radio/rx_success", "", "", "", "diagnostic");
        publishInverterSensor(inv, "RX Fail Receive Nothing", "radio/rx_fail_nothing", "", "", "", "diagnostic");
        publishInverterSensor(inv, "RX Fail Receive Partial", "radio/rx_fail_partial", "", "", "", "diagnostic");
        publishInverterSensor(inv, "RX Fail Receive Corrupt", "radio/rx_fail_corrupt", "", "", "", "diagnostic");
        publishInverterSensor(inv, "TX Re-Request Fragment", "radio/tx_re_request", "", "", "", "diagnostic");

        yield();

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
    if (type == TYPE_INV && fieldType.fieldId == FLD_PDC) {
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

        JsonDocument root;

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

        if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
            return;
        }

        String buffer;
        serializeJson(root, buffer);
        publish(configTopic, buffer);
    } else {
        publish(configTopic, "");
    }
}

void MqttHandleHassClass::publishInverterButton(std::shared_ptr<InverterAbstract> inv, const String& name, const String& icon, const String& category, const String& deviceClass, const String& subTopic, const String& payload)
{
    const String serial = inv->serialString();

    String buttonId = name;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "button/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + subTopic;

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = serial + "_" + buttonId;
    if (icon != "") {
        root["ic"] = icon;
    }
    if (deviceClass != "") {
        root["dev_cla"] = deviceClass;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["payload_press"] = payload;

    createInverterInfo(root, inv);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishInverterNumber(
    std::shared_ptr<InverterAbstract> inv, const String& name, const String& icon, const String& category,
    const String& commandTopic, const String& stateTopic, const String& unitOfMeasure,
    const int16_t min, const int16_t max, float step)
{
    const String serial = inv->serialString();

    String buttonId = name;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "number/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + commandTopic;
    const String statTopic = MqttSettings.getPrefix() + serial + "/" + stateTopic;

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = serial + "_" + buttonId;
    if (icon != "") {
        root["ic"] = icon;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["stat_t"] = statTopic;
    root["unit_of_meas"] = unitOfMeasure;
    root["min"] = min;
    root["max"] = max;
    root["step"] = step;

    createInverterInfo(root, inv);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& subTopic, const String& payload_on, const String& payload_off)
{
    const String serial = inv->serialString();

    String sensorId = name;
    sensorId.replace(" ", "_");
    sensorId.toLowerCase();

    const String configTopic = "binary_sensor/dtu_" + serial
        + "/" + sensorId
        + "/config";

    const String statTopic = MqttSettings.getPrefix() + serial + "/" + subTopic;

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = serial + "_" + sensorId;
    root["stat_t"] = statTopic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    createInverterInfo(root, inv);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishInverterSensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& subTopic, const String& unit_of_measure, const String& icon, const String& device_class, const String& category)
{
    const String serial = inv->serialString();

    String sensorId = name;
    sensorId.replace(" ", "_");
    sensorId.toLowerCase();

    const String configTopic = "sensor/dtu_" + serial
        + "/" + sensorId
        + "/config";

    const String statTopic = MqttSettings.getPrefix() + serial + "/" + subTopic;

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = serial + "_" + sensorId;
    if (device_class != "") {
        root["dev_cla"] = device_class;
    }
    if (category != "") {
        root["ent_cat"] = category;
    }
    if (icon != "") {
        root["ic"] = icon;
    }
    if (unit_of_measure != "") {
        root["unit_of_meas"] = unit_of_measure;
    }
    root["stat_t"] = statTopic;

    root["avty_t"] = MqttSettings.getPrefix() + Configuration.get().Mqtt.Lwt.Topic;

    const CONFIG_T& config = Configuration.get();
    root["pl_avail"] = config.Mqtt.Lwt.Value_Online;
    root["pl_not_avail"] = config.Mqtt.Lwt.Value_Offline;

    createInverterInfo(root, inv);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishDtuSensor(const String& name, const String& subTopic, const String& unit_of_measure, const String& icon, const String& device_class, const String& category)
{
    String id = name;
    id.toLowerCase();
    id.replace(" ", "_");
    String topic = subTopic;
    if (topic == "") {
        topic = id;
    }

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = getDtuUniqueId() + "_" + id;
    if (device_class != "") {
        root["dev_cla"] = device_class;
    }
    if (category != "") {
        root["ent_cat"] = category;
    }
    if (icon != "") {
        root["ic"] = icon;
    }
    if (unit_of_measure != "") {
        root["unit_of_meas"] = unit_of_measure;
    }
    root["stat_t"] = MqttSettings.getPrefix() + "dtu" + "/" + topic;

    root["avty_t"] = MqttSettings.getPrefix() + Configuration.get().Mqtt.Lwt.Topic;

    const CONFIG_T& config = Configuration.get();
    root["pl_avail"] = config.Mqtt.Lwt.Value_Online;
    root["pl_not_avail"] = config.Mqtt.Lwt.Value_Offline;

    createDtuInfo(root);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    const String configTopic = "sensor/" + getDtuUniqueId() + "/" + id + "/config";
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::publishDtuBinarySensor(const String& name, const String& subTopic, const String& payload_on, const String& payload_off, const String& device_class, const String& category)
{
    String id = name;
    id.toLowerCase();
    id.replace(" ", "_");

    String topic = subTopic;
    if (subTopic == "") {
        topic = String("dtu/") + "/" + id;
    }

    JsonDocument root;

    root["name"] = name;
    root["uniq_id"] = getDtuUniqueId() + "_" + id;
    root["stat_t"] = MqttSettings.getPrefix() + topic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    if (device_class != "") {
        root["dev_cla"] = device_class;
    }
    if (category != "") {
        root["ent_cat"] = category;
    }

    createDtuInfo(root);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    const String configTopic = "binary_sensor/" + getDtuUniqueId() + "/" + id + "/config";
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandleHassClass::createInverterInfo(JsonDocument& root, std::shared_ptr<InverterAbstract> inv)
{
    createDeviceInfo(
        root,
        inv->name(),
        inv->serialString(),
        getDtuUrl(),
        "OpenDTU",
        inv->typeName(),
        __COMPILED_GIT_HASH__,
        getDtuUniqueId());
}

void MqttHandleHassClass::createDtuInfo(JsonDocument& root)
{
    createDeviceInfo(
        root,
        NetworkSettings.getHostname(),
        getDtuUniqueId(),
        getDtuUrl(),
        "OpenDTU",
        "OpenDTU",
        __COMPILED_GIT_HASH__);
}

void MqttHandleHassClass::createDeviceInfo(
    JsonDocument& root,
    const String& name, const String& identifiers, const String& configuration_url,
    const String& manufacturer, const String& model, const String& sw_version,
    const String& via_device)
{
    auto object = root["dev"].to<JsonObject>();

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
    MqttSettings.publishGeneric(topic, payload, Configuration.get().Mqtt.Hass.Retain);
}

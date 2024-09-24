// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include "MqttHandleHass.h"
#include "MqttHandleInverter.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "Utils.h"
#include "__compiled_constants.h"
#include "defaults.h"

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
    publishDtuSensor("IP", "dtu/ip", "", "mdi:network-outline", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("WiFi Signal", "dtu/rssi", "dBm", "", DEVICE_CLS_SIGNAL_STRENGTH, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Uptime", "dtu/uptime", "s", "", DEVICE_CLS_DURATION, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Temperature", "dtu/temperature", "°C", "mdi:thermometer", DEVICE_CLS_TEMPERATURE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Heap Size", "dtu/heap/size", "Bytes", "mdi:memory", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Heap Free", "dtu/heap/free", "Bytes", "mdi:memory", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Largest Free Heap Block", "dtu/heap/maxalloc", "Bytes", "mdi:memory", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
    publishDtuSensor("Lifetime Minimum Free Heap", "dtu/heap/minfree", "Bytes", "mdi:memory", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);

    publishDtuSensor("Yield Total", "ac/yieldtotal", "kWh", "", DEVICE_CLS_ENERGY, CATEGORY_NONE);
    publishDtuSensor("Yield Day", "ac/yieldday", "Wh", "", DEVICE_CLS_ENERGY, CATEGORY_NONE);
    publishDtuSensor("AC Power", "ac/power", "W", "", DEVICE_CLS_PWR, CATEGORY_NONE);

    publishDtuBinarySensor("Status", config.Mqtt.Lwt.Topic, config.Mqtt.Lwt.Value_Online, config.Mqtt.Lwt.Value_Offline, DEVICE_CLS_CONNECTIVITY, CATEGORY_DIAGNOSTIC);

    // Loop all inverters
    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);

        publishInverterButton(inv, "Turn Inverter Off", "cmd/power", "0", "mdi:power-plug-off", DEVICE_CLS_NONE, CATEGORY_CONFIG);
        publishInverterButton(inv, "Turn Inverter On", "cmd/power", "1", "mdi:power-plug", DEVICE_CLS_NONE, CATEGORY_CONFIG);
        publishInverterButton(inv, "Restart Inverter", "cmd/restart", "1", "", DEVICE_CLS_RESTART, CATEGORY_CONFIG);
        publishInverterButton(inv, "Reset Radio Statistics", "cmd/reset_rf_stats", "1", "", DEVICE_CLS_NONE, CATEGORY_CONFIG);

        publishInverterNumber(inv, "Limit NonPersistent Relative", "status/limit_relative", "cmd/limit_nonpersistent_relative", 0, 100, 0.1, "%", "mdi:speedometer", CATEGORY_CONFIG);
        publishInverterNumber(inv, "Limit Persistent Relative", "status/limit_relative", "cmd/limit_persistent_relative", 0, 100, 0.1, "%", "mdi:speedometer", CATEGORY_CONFIG);

        publishInverterNumber(inv, "Limit NonPersistent Absolute", "status/limit_absolute", "cmd/limit_nonpersistent_absolute", 0, MAX_INVERTER_LIMIT, 1, "W", "mdi:speedometer", CATEGORY_CONFIG);
        publishInverterNumber(inv, "Limit Persistent Absolute", "status/limit_absolute", "cmd/limit_persistent_absolute", 0, MAX_INVERTER_LIMIT, 1, "W", "mdi:speedometer", CATEGORY_CONFIG);

        publishInverterBinarySensor(inv, "Reachable", "status/reachable", "1", "0", DEVICE_CLS_CONNECTIVITY, CATEGORY_DIAGNOSTIC);
        publishInverterBinarySensor(inv, "Producing", "status/producing", "1", "0", DEVICE_CLS_NONE, CATEGORY_NONE);

        publishInverterSensor(inv, "TX Requests", "radio/tx_request", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
        publishInverterSensor(inv, "RX Success", "radio/rx_success", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
        publishInverterSensor(inv, "RX Fail Receive Nothing", "radio/rx_fail_nothing", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
        publishInverterSensor(inv, "RX Fail Receive Partial", "radio/rx_fail_partial", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
        publishInverterSensor(inv, "RX Fail Receive Corrupt", "radio/rx_fail_corrupt", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);
        publishInverterSensor(inv, "TX Re-Request Fragment", "radio/tx_re_request", "", "", DEVICE_CLS_NONE, CATEGORY_DIAGNOSTIC);

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
        const char* stateCls = stateClass_name[fieldType.stateClsId];

        String name;
        if (type != TYPE_DC) {
            name = fieldName;
        } else {
            name = "CH" + chanNum + " " + fieldName;
        }

        String unit_of_measure = inv->Statistics()->getChannelFieldUnit(type, channel, fieldType.fieldId);

        JsonDocument root;
        createInverterInfo(root, inv);
        addCommonMetadata(root, unit_of_measure, "", fieldType.deviceClsId, CATEGORY_NONE);

        root["name"] = name;
        root["stat_t"] = stateTopic;
        root["uniq_id"] = serial + "_ch" + chanNum + "_" + fieldName;

        if (Configuration.get().Mqtt.Hass.Expire) {
            root["exp_aft"] = Hoymiles.getNumInverters() * max<uint32_t>(Hoymiles.PollInterval(), Configuration.get().Mqtt.PublishInterval) * inv->getReachableThreshold();
        }
        if (stateCls != 0) {
            root["stat_cla"] = stateCls;
        }

        publish(configTopic, root);
    } else {
        publish(configTopic, "");
    }
}

void MqttHandleHassClass::publishInverterButton(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& payload, const String& icon, const DeviceClassType device_class, const CategoryType category)
{
    const String serial = inv->serialString();

    String buttonId = name;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "button/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + state_topic;

    JsonDocument root;
    createInverterInfo(root, inv);
    addCommonMetadata(root, "", icon, device_class, category);

    root["name"] = name;
    root["uniq_id"] = serial + "_" + buttonId;
    root["cmd_t"] = cmdTopic;
    root["payload_press"] = payload;

    publish(configTopic, root);
}

void MqttHandleHassClass::publishInverterNumber(
    std::shared_ptr<InverterAbstract> inv, const String& name,
    const String& stateTopic, const String& command_topic,
    const int16_t min, const int16_t max, float step,
    const String& unit_of_measure, const String& icon, const CategoryType category)
{
    const String serial = inv->serialString();

    String buttonId = name;
    buttonId.replace(" ", "_");
    buttonId.toLowerCase();

    const String configTopic = "number/dtu_" + serial
        + "/" + buttonId
        + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + serial + "/" + command_topic;
    const String statTopic = MqttSettings.getPrefix() + serial + "/" + stateTopic;

    JsonDocument root;
    createInverterInfo(root, inv);
    addCommonMetadata(root, unit_of_measure, icon, DEVICE_CLS_NONE, category);

    root["name"] = name;
    root["uniq_id"] = serial + "_" + buttonId;
    root["cmd_t"] = cmdTopic;
    root["stat_t"] = statTopic;
    root["min"] = min;
    root["max"] = max;
    root["step"] = step;

    publish(configTopic, root);
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
    yield();
}

void MqttHandleHassClass::publish(const String& subtopic, const JsonDocument& doc)
{
    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return;
    }
    String buffer;
    serializeJson(doc, buffer);
    publish(subtopic, buffer);
}

void MqttHandleHassClass::addCommonMetadata(JsonDocument& doc, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category)
{
    if (unit_of_measure != "") {
        doc["unit_of_meas"] = unit_of_measure;
    }
    if (icon != "") {
        doc["ic"] = icon;
    }
    if (device_class != DEVICE_CLS_NONE) {
        doc["dev_cla"] = deviceClass_name[device_class];
    }
    if (category != CATEGORY_NONE) {
        doc["ent_cat"] = category_name[category];
    }
}

void MqttHandleHassClass::publishBinarySensor(JsonDocument& doc, const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category)
{
    String sensor_id = name;
    sensor_id.toLowerCase();
    sensor_id.replace(" ", "_");

    doc["name"] = name;
    doc["uniq_id"] = unique_id_prefix + "_" + sensor_id;
    doc["stat_t"] = MqttSettings.getPrefix() + state_topic;
    doc["pl_on"] = payload_on;
    doc["pl_off"] = payload_off;

    addCommonMetadata(doc, "", "", device_class, category);

    const String configTopic = "binary_sensor/" + root_device + "/" + sensor_id + "/config";
    publish(configTopic, doc);
}

void MqttHandleHassClass::publishDtuBinarySensor(const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category)
{
    const String dtuId = getDtuUniqueId();

    JsonDocument root;
    createDtuInfo(root);
    publishBinarySensor(root, dtuId, dtuId, name, state_topic, payload_on, payload_off, device_class, category);
}

void MqttHandleHassClass::publishInverterBinarySensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& payload_on, const String& payload_off, const DeviceClassType device_class, const CategoryType category)
{
    const String serial = inv->serialString();

    JsonDocument root;
    createInverterInfo(root, inv);
    publishBinarySensor(root, "dtu_" + serial, serial, name, serial + "/" + state_topic, payload_on, payload_off, device_class, category);
}

void MqttHandleHassClass::publishSensor(JsonDocument& doc, const String& root_device, const String& unique_id_prefix, const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category)
{
    String sensor_id = name;
    sensor_id.toLowerCase();
    sensor_id.replace(" ", "_");

    doc["name"] = name;
    doc["uniq_id"] = unique_id_prefix + "_" + sensor_id;
    doc["stat_t"] = MqttSettings.getPrefix() + state_topic;

    addCommonMetadata(doc, unit_of_measure, icon, device_class, category);

    const CONFIG_T& config = Configuration.get();
    doc["avty_t"] = MqttSettings.getPrefix() + config.Mqtt.Lwt.Topic;
    doc["pl_avail"] = config.Mqtt.Lwt.Value_Online;
    doc["pl_not_avail"] = config.Mqtt.Lwt.Value_Offline;

    const String configTopic = "sensor/" + root_device + "/" + sensor_id + "/config";
    publish(configTopic, doc);
}

void MqttHandleHassClass::publishDtuSensor(const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category)
{
    const String dtuId = getDtuUniqueId();

    JsonDocument root;
    createDtuInfo(root);
    publishSensor(root, dtuId, dtuId, name, state_topic, unit_of_measure, icon, device_class, category);
}

void MqttHandleHassClass::publishInverterSensor(std::shared_ptr<InverterAbstract> inv, const String& name, const String& state_topic, const String& unit_of_measure, const String& icon, const DeviceClassType device_class, const CategoryType category)
{
    const String serial = inv->serialString();

    JsonDocument root;
    createInverterInfo(root, inv);
    publishSensor(root, "dtu_" + serial, serial, name, serial + "/" + state_topic, unit_of_measure, icon, device_class, category);
}

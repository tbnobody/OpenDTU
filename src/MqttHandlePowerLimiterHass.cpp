// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "MqttHandlePowerLimiterHass.h"
#include "MqttHandleHass.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "MessageOutput.h"
#include "Utils.h"
#include "PowerLimiter.h"
#include "__compiled_constants.h"

MqttHandlePowerLimiterHassClass MqttHandlePowerLimiterHass;

void MqttHandlePowerLimiterHassClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&MqttHandlePowerLimiterHassClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
}

void MqttHandlePowerLimiterHassClass::loop()
{
    if (!Configuration.get().PowerLimiter.Enabled) {
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

void MqttHandlePowerLimiterHassClass::forceUpdate()
{
    _updateForced = true;
}

void MqttHandlePowerLimiterHassClass::publishConfig()
{
    auto const& config = Configuration.get();

    if (!config.Mqtt.Hass.Enabled) {
        return;
    }

    if (!MqttSettings.getConnected()) {
        return;
    }

    if (!config.PowerLimiter.Enabled) {
        return;
    }

    publishSelect("DPL Mode", "mdi:gauge", "config", "mode", "mode");

    if (!PowerLimiter.usesBatteryPoweredInverter()) {
        return;
    }

    // as this project revolves around Hoymiles inverters, 16 - 60 V is a reasonable voltage range
    publishNumber("DPL battery voltage start threshold", "mdi:battery-charging",
            "config", "threshold/voltage/start", "threshold/voltage/start", "V", 16, 60, 0.1);
    publishNumber("DPL battery voltage stop threshold", "mdi:battery-charging",
            "config", "threshold/voltage/stop", "threshold/voltage/stop", "V", 16, 60, 0.1);

    if (config.Vedirect.Enabled) {
        publishBinarySensor("full solar passthrough active",
            "mdi:transmission-tower-import",
            "full_solar_passthrough_active", "1", "0");

        publishNumber("DPL full solar passthrough start voltage",
                "mdi:transmission-tower-import", "config",
                "threshold/voltage/full_solar_passthrough_start",
                "threshold/voltage/full_solar_passthrough_start", "V", 16, 60, 0.1);
        publishNumber("DPL full solar passthrough stop voltage",
                "mdi:transmission-tower-import", "config",
                "threshold/voltage/full_solar_passthrough_stop",
                "threshold/voltage/full_solar_passthrough_stop", "V", 16, 60, 0.1);
    }

    if (config.Battery.Enabled && !config.PowerLimiter.IgnoreSoc) {
        publishNumber("DPL battery SoC start threshold", "mdi:battery-charging",
                "config", "threshold/soc/start", "threshold/soc/start", "%", 0, 100, 1.0);
        publishNumber("DPL battery SoC stop threshold", "mdi:battery-charging",
                "config", "threshold/soc/stop", "threshold/soc/stop", "%", 0, 100, 1.0);

        if (config.Vedirect.Enabled) {
            publishNumber("DPL full solar passthrough SoC",
                    "mdi:transmission-tower-import", "config",
                    "threshold/soc/full_solar_passthrough",
                    "threshold/soc/full_solar_passthrough", "%", 0, 100, 1.0);
        }
    }
}

void MqttHandlePowerLimiterHassClass::publishSelect(
    const char* caption, const char* icon, const char* category,
    const char* commandTopic, const char* stateTopic)
{

    String selectId = caption;
    selectId.replace(" ", "_");
    selectId.toLowerCase();

    const String configTopic = "select/" + MqttHandleHass.getDtuUniqueId() + "/" + selectId + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + "powerlimiter/cmd/" + commandTopic;
    const String statTopic = MqttSettings.getPrefix() + "powerlimiter/status/" + stateTopic;

    JsonDocument root;

    root["name"] = caption;
    root["uniq_id"] = MqttHandleHass.getDtuUniqueId() + "_" + selectId;
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["stat_t"] = statTopic;
    JsonArray options = root["options"].to<JsonArray>();
    options.add("0");
    options.add("1");
    options.add("2");

    createDeviceInfo(root);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandlePowerLimiterHassClass::publishNumber(
    const char* caption, const char* icon, const char* category,
    const char* commandTopic, const char* stateTopic, const char* unitOfMeasure,
    const int16_t min, const int16_t max, const float step)
{

    String numberId = caption;
    numberId.replace(" ", "_");
    numberId.toLowerCase();

    const String configTopic = "number/" + MqttHandleHass.getDtuUniqueId() + "/" + numberId + "/config";

    const String cmdTopic = MqttSettings.getPrefix() + "powerlimiter/cmd/" + commandTopic;
    const String statTopic = MqttSettings.getPrefix() + "powerlimiter/status/" + stateTopic;

    JsonDocument root;

    root["name"] = caption;
    root["uniq_id"] = MqttHandleHass.getDtuUniqueId() + "_" + numberId;
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    root["ent_cat"] = category;
    root["cmd_t"] = cmdTopic;
    root["stat_t"] = statTopic;
    root["unit_of_meas"] = unitOfMeasure;
    root["min"] = min;
    root["max"] = max;
    root["step"] = step;
    root["mode"] = "box";

    auto const& config = Configuration.get();
    if (config.Mqtt.Hass.Expire) {
        root["exp_aft"] = config.Mqtt.PublishInterval * 3;
    }

    createDeviceInfo(root);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}

void MqttHandlePowerLimiterHassClass::publishBinarySensor(
    const char* caption, const char* icon,
    const char* stateTopic, const char* payload_on, const char* payload_off)
{

    String numberId = caption;
    numberId.replace(" ", "_");
    numberId.toLowerCase();

    const String configTopic = "binary_sensor/" + MqttHandleHass.getDtuUniqueId() + "/" + numberId + "/config";

    const String statTopic = MqttSettings.getPrefix() + "powerlimiter/status/" + stateTopic;

    JsonDocument root;

    root["name"] = caption;
    root["uniq_id"] = MqttHandleHass.getDtuUniqueId() + "_" + numberId;
    if (strcmp(icon, "")) {
        root["ic"] = icon;
    }
    root["stat_t"] = statTopic;
    root["pl_on"] = payload_on;
    root["pl_off"] = payload_off;

    auto const& config = Configuration.get();
    if (config.Mqtt.Hass.Expire) {
        root["exp_aft"] = config.Mqtt.PublishInterval * 3;
    }

    createDeviceInfo(root);

    if (!Utils::checkJsonAlloc(root, __FUNCTION__, __LINE__)) {
        return;
    }

    String buffer;
    serializeJson(root, buffer);
    publish(configTopic, buffer);
}


void MqttHandlePowerLimiterHassClass::createDeviceInfo(JsonDocument& root)
{
    JsonObject object = root["dev"].to<JsonObject>();
    object["name"] = "Dynamic Power Limiter";
    object["ids"] = MqttHandleHass.getDtuUniqueId() + "_DPL";
    object["cu"] = MqttHandleHass.getDtuUrl();
    object["mf"] = "OpenDTU";
    object["mdl"] = "Dynamic Power Limiter";
    object["sw"] = __COMPILED_GIT_HASH__;
    object["via_device"] = MqttHandleHass.getDtuUniqueId();
}

void MqttHandlePowerLimiterHassClass::publish(const String& subtopic, const String& payload)
{
    String topic = Configuration.get().Mqtt.Hass.Topic;
    topic += subtopic;
    MqttSettings.publishGeneric(topic.c_str(), payload.c_str(), Configuration.get().Mqtt.Hass.Retain);
}

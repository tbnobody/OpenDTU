// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "MqttHandleDtu.h"
#include "Configuration.h"
#include "MqttSettings.h"
#include "NetworkSettings.h"
#include "RestartHelper.h"
#include <CpuTemperature.h>
#include <Hoymiles.h>

#undef TAG
static const char* TAG = "mqtt";

MqttHandleDtuClass MqttHandleDtu;

MqttHandleDtuClass::MqttHandleDtuClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleDtuClass::loop, this))
{
}

void MqttHandleDtuClass::init(Scheduler& scheduler)
{
    subscribeTopics();

    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleDtuClass::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    MqttSettings.publish("dtu/uptime", String(esp_timer_get_time() / 1000000));
    MqttSettings.publish("dtu/ip", NetworkSettings.localIP().toString());
    MqttSettings.publish("dtu/hostname", NetworkSettings.getHostname());
    MqttSettings.publish("dtu/heap/size", String(ESP.getHeapSize()));
    MqttSettings.publish("dtu/heap/free", String(ESP.getFreeHeap()));
    MqttSettings.publish("dtu/heap/minfree", String(ESP.getMinFreeHeap()));
    MqttSettings.publish("dtu/heap/maxalloc", String(ESP.getMaxAllocHeap()));
    if (NetworkSettings.NetworkMode() == network_mode::WiFi) {
        MqttSettings.publish("dtu/rssi", String(WiFi.RSSI()));
        MqttSettings.publish("dtu/bssid", WiFi.BSSIDstr());
    }

    float temperature = CpuTemperature.read();
    if (!std::isnan(temperature)) {
        MqttSettings.publish("dtu/temperature", String(temperature));
    }
}

void MqttHandleDtuClass::subscribeTopics()
{
    String const& prefix = MqttSettings.getPrefix();

    auto subscribe = [&prefix, this](char const* subTopic, Topic t) {
        String fullTopic(prefix + _cmdtopic.data() + subTopic);
        MqttSettings.subscribe(fullTopic.c_str(), 0,
            std::bind(&MqttHandleDtuClass::onMqttMessage, this,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));
    };

    for (auto const& s : _subscriptions) {
        subscribe(s.first.data(), s.second);
    }
}

void MqttHandleDtuClass::unsubscribeTopics()
{
    String const& prefix = MqttSettings.getPrefix() + _cmdtopic.data();
    for (auto const& s : _subscriptions) {
        MqttSettings.unsubscribe(prefix + s.first.data());
    }
}

void MqttHandleDtuClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* payload, const size_t len)
{
    std::string strValue(reinterpret_cast<const char*>(payload), len);
    float payload_val = -1;
    try {
        payload_val = std::stof(strValue);
    } catch (std::invalid_argument const& e) {
        ESP_LOGW(TAG, "MQTT handler: cannot parse payload of topic '%s' as float: %s",
            topic, strValue.c_str());
        return;
    }

    // Match "dtu/cmd/restart" in the topic
    const CONFIG_T& config = Configuration.get();
    String restartTopic = String(config.Mqtt.Topic) + _cmdtopic.data() + "restart";

    if (restartTopic == topic) {
        ESP_LOGI(TAG, "Restart OpenDTU");
        if (!properties.retain && payload_val == 1) {
            RestartHelper.triggerRestart();
        } else {
            ESP_LOGW(TAG, "Ignored because retained or numeric value not '1'");
        }
    }
}

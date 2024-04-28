// SPDX-License-Identifier: GPL-2.0-or-later
#include "MqttHandlePowerMeter.h"
#include "MessageOutput.h"
#include "MqttSettings.h"

#define TOPIC_SUB_RESET "power_meter_reset"

MqttHandlePowerMeterClass MqttHandlePowerMeter;

namespace {
}

MqttHandlePowerMeterClass::MqttHandlePowerMeterClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandlePowerMeterClass::loop, this))
{
}

void MqttHandlePowerMeterClass::init(Scheduler& scheduler)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    String topic = MqttSettings.getPrefix();
    topic.concat("+/cmd/" TOPIC_SUB_RESET);
    MqttSettings.subscribe(topic, 0, std::bind(&MqttHandlePowerMeterClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));

    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandlePowerMeterClass::loop()
{
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !JsyMk.isInitialised() || JsyMk.getLastUpdate() == _lastUpdate) {
        _loopTask.forceNextIteration();
        return;
    }

    _lastUpdate = JsyMk.getLastUpdate();

    // Loop all channels
    for (size_t i = 0; i < JsyMk.getChannelNumber(); ++i) {
        publishField(i, Field_t::VOLTAGE);
        publishField(i, Field_t::CURRENT);
        publishField(i, Field_t::POWER);
        publishField(i, Field_t::POWER_FACTOR);
        publishField(i, Field_t::FREQUENCY);
        publishField(i, Field_t::NEGATIVE);
        publishField(i, Field_t::TOTAL_POSITIVE_ENERGY);
        publishField(i, Field_t::TOTAL_NEGATIVE_ENERGY);

        yield();
    }
}

void MqttHandlePowerMeterClass::publishField(size_t channel, const Field_t fieldId)
{
    const String topic = getTopic(channel, fieldId);
    if (topic.isEmpty())
        return;

    MqttSettings.publish(topic, JsyMk.getFieldString(channel, fieldId));
}

String MqttHandlePowerMeterClass::getTopic(size_t channel, const Field_t fieldId)
{
    String model = JsyMk.getFieldString(channel, Field_t::MODEL);
    String name = JsyMk.getFieldName(channel, fieldId);

    if (model.isEmpty() || name.isEmpty())
        return {};

    String sensorId = name;
    sensorId.replace(" ", "_");
    sensorId.toLowerCase();

    return model + "/" + String(channel) + "/" + sensorId;
}

void MqttHandlePowerMeterClass::onMqttMessage(const espMqttClientTypes::MessageProperties& properties, const char* topic, const uint8_t* /*payload*/, const size_t /*len*/, const size_t /*index*/, const size_t /*total*/)
{
    const CONFIG_T& config = Configuration.get();

    std::string_view tokenTopic = topic;
    tokenTopic.remove_prefix(strlen(config.Mqtt.Topic));

    auto findNextToken = [&]() -> std::string_view {
        std::string_view result;

        size_t pos = tokenTopic.find('/');
        if (pos == std::string_view::npos) {
            result = tokenTopic;
            tokenTopic = {};
        } else {
            result = tokenTopic.substr(pos);
            tokenTopic.remove_prefix(pos + 1);
        }

        return result;
    };

    std::string_view model = findNextToken();
    std::string_view subtopic = findNextToken();
    std::string_view setting = findNextToken();

    if (model.empty() || subtopic.empty() || setting.empty() || subtopic != "cmd") {
        return;
    }

    if (setting == TOPIC_SUB_RESET) {
        MessageOutput.println(TOPIC_SUB_RESET);
        if (JsyMk.isInitialised())
            JsyMk.reset();
    }
}

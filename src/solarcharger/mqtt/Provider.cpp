// SPDX-License-Identifier: GPL-2.0-or-later
#include <solarcharger/mqtt/Provider.h>
#include <Configuration.h>
#include <MqttSettings.h>
#include <Utils.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "solarCharger";
static const char* SUBTAG = "MQTT";

namespace SolarChargers::Mqtt {

bool Provider::init()
{
    auto const& config = Configuration.get().SolarCharger.Mqtt;

    _outputPowerTopic = config.PowerTopic;
    _outputCurrentTopic = config.CurrentTopic;
    _outputVoltageTopic = config.VoltageTopic;

    bool configValid = !config.CalculateOutputPower && !_outputPowerTopic.isEmpty();
    if (config.CalculateOutputPower) {
        configValid = !_outputCurrentTopic.isEmpty() && !_outputVoltageTopic.isEmpty();
    }

    if (!configValid) {
        DTU_LOGE("Init failed. "
               "switch 'calculate output power' %s, power topic %s, "
               "current topic %s, voltage topic %s",
               config.CalculateOutputPower ? "enabled" : "disabled",
               _outputPowerTopic.isEmpty() ? "empty" : "available",
               _outputCurrentTopic.isEmpty() ? "empty" : "available",
               _outputVoltageTopic.isEmpty() ? "empty" : "available"
        );
        return false;
    }

    if (!_outputPowerTopic.isEmpty()
        && !config.CalculateOutputPower) {
        _subscribedTopics.push_back(_outputPowerTopic);

        MqttSettings.subscribe(_outputPowerTopic, 0/*QoS*/,
                std::bind(&Provider::onMqttMessageOutputPower,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    config.PowerJsonPath)
                );

        DTU_LOGI("Subscribed to '%s' for ouput_power readings", _outputPowerTopic.c_str());
    }

    if (!_outputCurrentTopic.isEmpty()) {
        _subscribedTopics.push_back(_outputCurrentTopic);

        MqttSettings.subscribe(_outputCurrentTopic, 0/*QoS*/,
                std::bind(&Provider::onMqttMessageOutputCurrent,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    config.CurrentJsonPath)
                );

        DTU_LOGI("Subscribed to '%s' for output_current readings", _outputCurrentTopic.c_str());
    }

    if (!_outputVoltageTopic.isEmpty()) {
        _subscribedTopics.push_back(_outputVoltageTopic);

        MqttSettings.subscribe(_outputVoltageTopic, 0/*QoS*/,
                std::bind(&Provider::onMqttMessageOutputVoltage,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    config.VoltageJsonPath)
                );

        DTU_LOGI("Subscribed to '%s' for ouput_voltage readings", _outputVoltageTopic.c_str());
    }

    return true;
}

void Provider::deinit()
{
    for (auto const& topic : _subscribedTopics) {
        MqttSettings.unsubscribe(topic);
    }
    _subscribedTopics.clear();
}

void Provider::onMqttMessageOutputPower(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len,
            char const* jsonPath) const
{
    auto outputPower = Utils::getNumericValueFromMqttPayload<float>("SolarChargers::Mqtt",
            std::string(reinterpret_cast<const char*>(payload), len), topic,
            jsonPath);

    if (!outputPower.has_value()) { return; }

    auto const& config = Configuration.get().SolarCharger.Mqtt;
    using Unit_t = SolarChargerMqttConfig::WattageUnit;
    switch (config.PowerUnit) {
        case Unit_t::MilliWatts:
            *outputPower /= 1000;
            break;
        case Unit_t::KiloWatts:
            *outputPower *= 1000;
            break;
        default:
            break;
    }

    _stats->setOutputPowerWatts(*outputPower);

    DTU_LOGD("Updated output_power to %.1f from '%s'", *outputPower, topic);
}

void Provider::onMqttMessageOutputVoltage(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len,
            char const* jsonPath) const
{
    auto outputVoltage = Utils::getNumericValueFromMqttPayload<float>("SolarChargers::Mqtt",
            std::string(reinterpret_cast<const char*>(payload), len), topic,
            jsonPath);

    if (!outputVoltage.has_value()) { return; }

    auto const& config = Configuration.get().SolarCharger.Mqtt;
    using Unit_t = SolarChargerMqttConfig::VoltageUnit;
    switch (config.VoltageTopicUnit) {
        case Unit_t::DeciVolts:
            *outputVoltage /= 10;
            break;
        case Unit_t::CentiVolts:
            *outputVoltage /= 100;
            break;
        case Unit_t::MilliVolts:
            *outputVoltage /= 1000;
            break;
        default:
            break;
    }

    // since this project is revolving around Hoymiles microinverters, which can
    // only handle up to 65V of input voltage at best, it is safe to assume that
    // an even higher voltage is implausible.
    if (*outputVoltage < 0 || *outputVoltage > 65) {
        DTU_LOGW("Implausible output_voltage '%.2f' in topic '%s'", *outputVoltage, topic);
        return;
    }

    _stats->setOutputVoltage(*outputVoltage);

    DTU_LOGD("Updated output_voltage to %.2f from '%s'", *outputVoltage, topic);
}

void Provider::onMqttMessageOutputCurrent(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len,
            char const* jsonPath) const
{
    auto outputCurrent = Utils::getNumericValueFromMqttPayload<float>("SolarChargers::Mqtt",
            std::string(reinterpret_cast<const char*>(payload), len), topic,
            jsonPath);

    if (!outputCurrent.has_value()) { return; }

    auto const& config = Configuration.get().SolarCharger.Mqtt;
    using Unit_t = SolarChargerMqttConfig::AmperageUnit;
    switch (config.CurrentUnit) {
        case Unit_t::MilliAmps:
            *outputCurrent /= 1000;
            break;
        default:
            break;
    }

    _stats->setOutputCurrent(*outputCurrent);

    DTU_LOGD("Updated output_current to %.2f from '%s'", *outputCurrent, topic);
}

} // namespace SolarChargers::Mqtt

#include <functional>

#include "Configuration.h"
#include "MqttBattery.h"
#include "MqttSettings.h"
#include "MessageOutput.h"

bool MqttBattery::init(bool verboseLogging)
{
    _verboseLogging = verboseLogging;

    auto const& config = Configuration.get();

    _socTopic = config.Battery.MqttSocTopic;
    if (!_socTopic.isEmpty()) {
        MqttSettings.subscribe(_socTopic, 0/*QoS*/,
                std::bind(&MqttBattery::onMqttMessageSoC,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5, std::placeholders::_6)
                );

        if (_verboseLogging) {
            MessageOutput.printf("MqttBattery: Subscribed to '%s' for SoC readings\r\n",
                _socTopic.c_str());
        }
    }

    _voltageTopic = config.Battery.MqttVoltageTopic;
    if (!_voltageTopic.isEmpty()) {
        MqttSettings.subscribe(_voltageTopic, 0/*QoS*/,
                std::bind(&MqttBattery::onMqttMessageVoltage,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5, std::placeholders::_6)
                );

        if (_verboseLogging) {
            MessageOutput.printf("MqttBattery: Subscribed to '%s' for voltage readings\r\n",
                _voltageTopic.c_str());
        }
    }

    return true;
}

void MqttBattery::deinit()
{
    if (!_voltageTopic.isEmpty()) {
        MqttSettings.unsubscribe(_voltageTopic);
    }

    if (!_socTopic.isEmpty()) {
        MqttSettings.unsubscribe(_socTopic);
    }
}

std::optional<float> MqttBattery::getFloat(std::string const& src, char const* topic) {
    float res = 0;

    try {
        res = std::stof(src);
    }
    catch(std::invalid_argument const& e) {
        MessageOutput.printf("MqttBattery: Cannot parse payload '%s' in topic '%s' as float\r\n",
                src.c_str(), topic);
        return std::nullopt;
    }

    return res;
}

void MqttBattery::onMqttMessageSoC(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len, size_t index, size_t total)
{
    auto soc = getFloat(std::string(reinterpret_cast<const char*>(payload), len), topic);
    if (!soc.has_value()) { return; }

    if (*soc < 0 || *soc > 100) {
        MessageOutput.printf("MqttBattery: Implausible SoC '%.2f' in topic '%s'\r\n",
                *soc, topic);
        return;
    }

    _stats->setSoC(static_cast<uint8_t>(*soc));

    if (_verboseLogging) {
        MessageOutput.printf("MqttBattery: Updated SoC to %d from '%s'\r\n",
                static_cast<uint8_t>(*soc), topic);
    }
}

void MqttBattery::onMqttMessageVoltage(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len, size_t index, size_t total)
{
    auto voltage = getFloat(std::string(reinterpret_cast<const char*>(payload), len), topic);
    if (!voltage.has_value()) { return; }

    // since this project is revolving around Hoymiles microinverters, which can
    // only handle up to 65V of input voltage at best, it is safe to assume that
    // an even higher voltage is implausible.
    if (*voltage < 0 || *voltage > 65) {
        MessageOutput.printf("MqttBattery: Implausible voltage '%.2f' in topic '%s'\r\n",
                *voltage, topic);
        return;
    }

    _stats->setVoltage(*voltage, millis());

    if (_verboseLogging) {
        MessageOutput.printf("MqttBattery: Updated voltage to %.2f from '%s'\r\n",
                *voltage, topic);
    }
}

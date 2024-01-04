#include <functional>

#include "Configuration.h"
#include "MqttBattery.h"
#include "MqttSettings.h"
#include "MessageOutput.h"

bool MqttBattery::init(bool verboseLogging)
{
    _verboseLogging = verboseLogging;

    auto const& config = Configuration.get();
    _socTopic = config.Battery.MqttTopic;

    if (_socTopic.isEmpty()) { return false; }

    MqttSettings.subscribe(_socTopic, 0/*QoS*/,
            std::bind(&MqttBattery::onMqttMessage,
                this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4,
                std::placeholders::_5, std::placeholders::_6)
            );

    if (_verboseLogging) {
        MessageOutput.printf("MqttBattery: Subscribed to '%s'\r\n",
            _socTopic.c_str());
    }

    return true;
}

void MqttBattery::deinit()
{
    if (_socTopic.isEmpty()) { return; }
    MqttSettings.unsubscribe(_socTopic);
}

void MqttBattery::onMqttMessage(espMqttClientTypes::MessageProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len, size_t index, size_t total)
{
    float soc = 0;
    std::string value(reinterpret_cast<const char*>(payload), len);

    try {
        soc = std::stof(value);
    }
    catch(std::invalid_argument const& e) {
        MessageOutput.printf("MqttBattery: Cannot parse payload '%s' in topic '%s' as float\r\n",
                value.c_str(), topic);
        return;
    }

    if (soc < 0 || soc > 100) {
        MessageOutput.printf("MqttBattery: Implausible SoC '%.2f' in topic '%s'\r\n",
                soc, topic);
        return;
    }

    _stats->setSoC(static_cast<uint8_t>(soc));

    if (_verboseLogging) {
        MessageOutput.printf("MqttBattery: Updated SoC to %d from '%s'\r\n",
                static_cast<uint8_t>(soc), topic);
    }
}

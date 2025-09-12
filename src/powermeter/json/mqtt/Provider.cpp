// SPDX-License-Identifier: GPL-2.0-or-later
#include <powermeter/json/mqtt/Provider.h>
#include <MqttSettings.h>
#include <ArduinoJson.h>
#include <Utils.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "powerMeter";
static const char* SUBTAG = "MQTT";

namespace PowerMeters::Json::Mqtt {

bool Provider::init()
{
    auto subscribe = [this](PowerMeterMqttValue const& val, uint8_t phaseIndex) {
        char const* topic = val.Topic;
        if (strlen(topic) == 0) { return; }
        MqttSettings.subscribe(topic, 0,
                std::bind(&Provider::onMessage,
                    this, std::placeholders::_1, std::placeholders::_2,
                    std::placeholders::_3, std::placeholders::_4,
                    phaseIndex, &val)
                );
        _mqttSubscriptions.push_back(topic);
    };

    for (uint8_t i = 0; i < POWERMETER_MQTT_MAX_VALUES; ++i) {
        subscribe(_cfg.Values[i], i);
    }

    return _mqttSubscriptions.size() > 0;
}

Provider::~Provider()
{
    for (auto const& t: _mqttSubscriptions) { MqttSettings.unsubscribe(t); }
    _mqttSubscriptions.clear();
}

void Provider::onMessage(Provider::MsgProperties const& properties,
        char const* topic, uint8_t const* payload, size_t len,
        uint8_t const phaseIndex, PowerMeterMqttValue const* cfg)
{
    auto extracted = Utils::getNumericValueFromMqttPayload<float>("PowerMeters::Json::Mqtt",
            std::string(reinterpret_cast<const char*>(payload), len), topic,
            cfg->JsonPath);

    if (!extracted.has_value()) { return; }

    float newValue = *extracted;

    using Unit_t = PowerMeterMqttValue::Unit;
    switch (cfg->PowerUnit) {
        case Unit_t::MilliWatts:
            newValue /= 1000;
            break;
        case Unit_t::KiloWatts:
            newValue *= 1000;
            break;
        default:
            break;
    }

    if (cfg->SignInverted) { newValue *= -1; }

    {
        auto scopedLock = _dataCurrent.lock();
        switch (phaseIndex) {
            case 0:
                _dataCurrent.add<DataPointLabel::PowerL1>(newValue);
                break;

            case 1:
                _dataCurrent.add<DataPointLabel::PowerL2>(newValue);
                break;

            case 2:
                _dataCurrent.add<DataPointLabel::PowerL3>(newValue);
                break;

            default:
                break;
        }
    }

    DTU_LOGD("Topic '%s': new value: %5.2f, total: %5.2f",
            topic, newValue, getPowerTotal());
}

} // namespace PowerMeters::Json::Mqtt

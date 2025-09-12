// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <battery/Provider.h>
#include <battery/zendure/Stats.h>
#include <battery/zendure/HassIntegration.h>
#include <battery/zendure/Constants.h>
#include <espMqttClient.h>

namespace Batteries::Zendure {

class Provider : public ::Batteries::Provider {
public:
    Provider();
    bool init() final;
    void deinit() final;
    void loop() final;
    std::shared_ptr<::Batteries::Stats> getStats() const final { return _stats; }
    std::shared_ptr<::Batteries::HassIntegration> getHassIntegration() final { return _hassIntegration; }

private:
    uint16_t setOutputLimit(uint16_t limit) const;
    uint16_t setInverterMax(uint16_t limit) const;
    void shutdown() const;

    void checkChargeThrough(uint32_t predictHours = 0U);

    void timesync();
    static String parseVersion(uint32_t version);
    uint16_t calcOutputLimit(uint16_t limit) const;
    void setTargetSoCs(const float soc_min, const float soc_max);
    void writeSettings();

    uint32_t _lastUpdate = 0;
    std::shared_ptr<Stats> _stats = std::make_shared<Stats>();
    std::shared_ptr<HassIntegration> _hassIntegration;


    void calculateEfficiency();
    void calculateFullChargeAge();
    void publishProperty(const String& topic, const String& property, const String& value) const;
    template<typename... Arg>
    void publishProperties(const String& topic, Arg&&... args) const;

    void setSoC(const float soc, const uint32_t timestamp = 0, const uint8_t precision = 2);
    void setChargeThroughState(const ChargeThroughState value, const bool publish = true);

    void rescheduleSunCalc() { _nextSunCalc = 0; }
    bool alive() const { return _stats->getAgeSeconds() < ZENDURE_ALIVE_SECONDS; }

    void publishPersistentSettings(const char* subtopic, const String& payload);

    uint32_t _rateFullUpdateMs = 0;
    uint64_t _nextFullUpdate = 0;

    uint32_t _rateTimesyncMs = 0;
    uint64_t _nextTimesync = 0;

    uint32_t _rateSunCalcMs = 0;
    uint64_t _nextSunCalc = 0;

    uint32_t _messageCounter = 0;

    String _deviceId = String();

    String _baseTopic = String();
    String _topicLog = String();
    String _topicReadReply = String();
    String _topicReport = String();
    String _topicRead = String();
    String _topicWrite = String();
    String _topicTimesync = String();
    String _topicPersistentSettings = String();

    String _payloadSettings = String();
    String _payloadFullUpdate = String();

    bool _full_log_supported = false;

    void onMqttMessageReport(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

    void onMqttMessageLog(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

    void onMqttMessageTimesync(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

    void onMqttMessagePersistentSettings(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

};

} // namespace Batteries::Zendure

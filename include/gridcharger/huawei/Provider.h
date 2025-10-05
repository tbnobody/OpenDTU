// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <gridcharger/Provider.h>
#include <gridcharger/huawei/HardwareInterface.h>
#include <gridcharger/huawei/DataPoints.h>
#include <gridcharger/huawei/Stats.h>
#include <espMqttClient.h>
#include <frozen/map.h>
#include <frozen/string.h>
#include <deque>
#include <functional>
#include <mutex>

namespace GridChargers::Huawei {

// Modes of operation
#define HUAWEI_MODE_OFF 0
#define HUAWEI_MODE_ON 1
#define HUAWEI_MODE_AUTO_EXT 2
#define HUAWEI_MODE_AUTO_INT 3

class Provider : public ::GridChargers::Provider {
public:
    bool init() final;
    void deinit() final;
    void loop() final;

    std::shared_ptr<::GridChargers::Stats> getStats() const final { return _stats; }
    bool getAutoPowerStatus() const final { return _autoPowerEnabled; };

    void setProduction(bool enable);
    void setParameter(float val, HardwareInterface::Setting setting);

    // determined through trial and error (voltage limits, R4850G2)
    // and some educated guessing (current limits, no R4875 at hand)
    static constexpr float MIN_ONLINE_VOLTAGE = 41.0f;
    static constexpr float MAX_ONLINE_VOLTAGE = 58.6f;
    static constexpr float MIN_ONLINE_CURRENT = 0.0f;
    static constexpr float MAX_ONLINE_CURRENT = 84.0f;
    static constexpr float MIN_OFFLINE_VOLTAGE = 48.0f;
    static constexpr float MAX_OFFLINE_VOLTAGE = 58.4f;
    static constexpr float MIN_OFFLINE_CURRENT = 0.0f;
    static constexpr float MAX_OFFLINE_CURRENT = 84.0f;
    static constexpr float MIN_INPUT_CURRENT_LIMIT = 0.0f;
    static constexpr float MAX_INPUT_CURRENT_LIMIT = 40.0f;

private:
    void _setParameter(float val, HardwareInterface::Setting setting, bool pollFeedback = false);
    void _setProduction(bool enable) const;

    void setFan(bool online, bool fullSpeed);
    void setMode(uint8_t mode);

    // these control the pin named "power", which in turn is supposed to control
    // a relay (or similar) to enable or disable the PSU using it's slot detect
    // pins.
    void enableOutput();
    void disableOutput();
    gpio_num_t _huaweiPower;

    Task _loopTask;
    std::unique_ptr<HardwareInterface> _upHardwareInterface;

    std::mutex _mutex;
    std::optional<bool> _oOutputEnabled;
    uint8_t _mode = HUAWEI_MODE_AUTO_EXT;

    DataPointContainer _dataPoints;
    std::shared_ptr<Stats> _stats = std::make_shared<Stats>();

    uint32_t _outputCurrentOnSinceMillis;         // Timestamp since when the PSU was idle at zero amps
    uint32_t _nextAutoModePeriodicIntMillis;      // When to set the next output voltage in automatic mode
    uint32_t _lastPowerMeterUpdateReceivedMillis; // Timestamp of last seen power meter value
    uint32_t _autoModeBlockedTillMillis = 0;      // Timestamp to block running auto mode for some time

    uint8_t _autoPowerEnabledCounter = 0;
    bool _autoPowerEnabled = false;
    bool _batteryEmergencyCharging = false;

    enum class Topic : unsigned {
        LimitOnlineVoltage,
        LimitOnlineCurrent,
        LimitOfflineVoltage,
        LimitOfflineCurrent,
        LimitInputCurrent,
        Mode,
        Production,
        FanOnlineFullSpeed,
        FanOfflineFullSpeed
    };

    void subscribeTopics();
    static void unsubscribeTopics();

    static constexpr frozen::string _cmdtopic = "huawei/cmd/";
    static constexpr frozen::map<frozen::string, Topic, 9> _subscriptions = {
        { "limit_online_voltage",   Topic::LimitOnlineVoltage },
        { "limit_online_current",   Topic::LimitOnlineCurrent },
        { "limit_offline_voltage",  Topic::LimitOfflineVoltage },
        { "limit_offline_current",  Topic::LimitOfflineCurrent },
        { "limit_input_current",    Topic::LimitInputCurrent },
        { "mode",                   Topic::Mode },
        { "production",             Topic::Production },
        { "fan_online_full_speed",  Topic::FanOnlineFullSpeed },
        { "fan_offline_full_speed", Topic::FanOfflineFullSpeed },
    };

    void onMqttMessage(Topic enumTopic,
            const espMqttClientTypes::MessageProperties& properties,
            const char* topic, const uint8_t* payload, size_t len);
};


} // namespace GridChargers::Huawei

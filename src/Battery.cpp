// SPDX-License-Identifier: GPL-2.0-or-later
#include "Battery.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "PylontechCanReceiver.h"
#include "JkBmsController.h"
#include "VictronSmartShunt.h"

BatteryClass Battery;

std::shared_ptr<BatteryStats const> BatteryClass::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) {
        static auto sspDummyStats = std::make_shared<BatteryStats>();
        return sspDummyStats;
    }

    return _upProvider->getStats();
}

void BatteryClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&BatteryClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void BatteryClass::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    CONFIG_T& config = Configuration.get();
    if (!config.Battery.Enabled) { return; }

    bool verboseLogging = config.Battery.VerboseLogging;

    switch (config.Battery.Provider) {
        case 0:
            _upProvider = std::make_unique<PylontechCanReceiver>();
            if (!_upProvider->init(verboseLogging)) { _upProvider = nullptr; }
            break;
        case 1:
            _upProvider = std::make_unique<JkBms::Controller>();
            if (!_upProvider->init(verboseLogging)) { _upProvider = nullptr; }
            break;
        case 3:
            _upProvider = std::make_unique<VictronSmartShunt>();
            if (!_upProvider->init(verboseLogging)) { _upProvider = nullptr; }
            break;
        default:
            MessageOutput.printf("Unknown battery provider: %d\r\n", config.Battery.Provider);
            break;
    }
}


void BatteryClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    CONFIG_T& config = Configuration.get();

    if (!MqttSettings.getConnected()
            || (millis() - _lastMqttPublish) < (config.Mqtt.PublishInterval * 1000)) {
        return;
    }

    _upProvider->getStats()->mqttPublish();

    _lastMqttPublish = millis();
}

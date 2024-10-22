// SPDX-License-Identifier: GPL-2.0-or-later
#include "Battery.h"
#include "MessageOutput.h"
#include "PylontechCanReceiver.h"
#include "SBSCanReceiver.h"
#include "JkBmsController.h"
#include "VictronSmartShunt.h"
#include "MqttBattery.h"
#include "PytesCanReceiver.h"

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
            break;
        case 1:
            _upProvider = std::make_unique<JkBms::Controller>();
            break;
        case 2:
            _upProvider = std::make_unique<MqttBattery>();
            break;
        case 3:
            _upProvider = std::make_unique<VictronSmartShunt>();
            break;
        case 4:
            _upProvider = std::make_unique<PytesCanReceiver>();
            break;
        case 5:
            _upProvider = std::make_unique<SBSCanReceiver>();
            break;
        default:
            MessageOutput.printf("[Battery] Unknown provider: %d\r\n", config.Battery.Provider);
            return;
    }

    if (!_upProvider->init(verboseLogging)) { _upProvider = nullptr; }
}

void BatteryClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    _upProvider->getStats()->mqttLoop();
}

float BatteryClass::getDischargeCurrentLimit()
{
    CONFIG_T& config = Configuration.get();

    if (!config.Battery.EnableDischargeCurrentLimit) { return FLT_MAX; }

    auto dischargeCurrentLimit = config.Battery.DischargeCurrentLimit;
    auto dischargeCurrentValid = dischargeCurrentLimit > 0.0f;

    auto statsCurrentLimit = getStats()->getDischargeCurrentLimit();
    auto statsLimitValid = config.Battery.UseBatteryReportedDischargeCurrentLimit
        && statsCurrentLimit >= 0.0f
        && getStats()->getDischargeCurrentLimitAgeSeconds() <= 60;

    if (statsLimitValid && dischargeCurrentValid) {
        // take the lowest limit
        return min(statsCurrentLimit, dischargeCurrentLimit);
    }

    if (statsLimitValid) {
        return statsCurrentLimit;
    }

    if (dischargeCurrentValid) {
        return dischargeCurrentLimit;
    }

    return FLT_MAX;
}

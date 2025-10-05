// SPDX-License-Identifier: GPL-2.0-or-later

#include <gridcharger/Controller.h>
#include <gridcharger/DummyStats.h>
#include <gridcharger/huawei/Provider.h>
#include <Configuration.h>
#include <MqttSettings.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "gridCharger";
static const char* SUBTAG = "Controller";

GridChargers::Controller GridCharger;

namespace GridChargers {

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    auto const& config = Configuration.get();
    if (!config.GridCharger.Enabled) { return; }

    switch (config.GridCharger.Provider) {
        case GridChargerProviderType::HUAWEI:
            _upProvider = std::make_unique<::GridChargers::Huawei::Provider>();
            break;
        default:
            DTU_LOGW("Unknown provider: %d\r\n", config.GridCharger.Provider);
            return;
    }

    if (!_upProvider->init()) { _upProvider = nullptr; }
}

void Controller::loop() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    _upProvider->getStats()->mqttLoop();
}

template<typename T>
T* Controller::getProvider() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_upProvider) return nullptr;
    return static_cast<T*>(_upProvider.get());
}

bool Controller::getAutoPowerStatus() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return false; }

    return _upProvider->getAutoPowerStatus();
}

std::shared_ptr<Stats const> Controller::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) {
        static auto sspDummyStats = std::make_shared<DummyStats>();
        return sspDummyStats;
    }

    return _upProvider->getStats();
}

// Template instantiations
template GridChargers::Huawei::Provider* Controller::getProvider<GridChargers::Huawei::Provider>() const;

} // namespace GridChargers

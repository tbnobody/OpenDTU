// SPDX-License-Identifier: GPL-2.0-or-later
#include <battery/Controller.h>
#include <battery/jbdbms/Provider.h>
#include <battery/jkbms/Provider.h>
#include <battery/mqtt/Provider.h>
#include <battery/pylontech/Provider.h>
#include <battery/pytes/Provider.h>
#include <battery/sbs/Provider.h>
#include <battery/victronsmartshunt/Provider.h>
#include <battery/zendure/Provider.h>
#include <Configuration.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "battery";
static const char* SUBTAG = "Controller";

Batteries::Controller Battery;

namespace Batteries {

std::shared_ptr<Stats const> Controller::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) {
        static auto sspDummyStats = std::make_shared<Stats>();
        return sspDummyStats;
    }

    return _upProvider->getStats();
}

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
    if (!config.Battery.Enabled) { return; }

    switch (config.Battery.Provider) {
        case 0:
            _upProvider = std::make_unique<Pylontech::Provider>();
            break;
        case 1:
            _upProvider = std::make_unique<JkBms::Provider>();
            break;
        case 2:
            _upProvider = std::make_unique<Mqtt::Provider>();
            break;
        case 3:
            _upProvider = std::make_unique<VictronSmartShunt::Provider>();
            break;
        case 4:
            _upProvider = std::make_unique<Pytes::Provider>();
            break;
        case 5:
            _upProvider = std::make_unique<SBS::Provider>();
            break;
        case 6:
            _upProvider = std::make_unique<JbdBms::Provider>();
            break;
        case 7:
            _upProvider = std::make_unique<Zendure::Provider>();
            break;
        default:
            DTU_LOGE("Unknown provider: %d", config.Battery.Provider);
            return;
    }

    if (!_upProvider->init()) { _upProvider = nullptr; }
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    _upProvider->getStats()->mqttLoop();

    auto spHassIntegration = _upProvider->getHassIntegration();
    if (spHassIntegration) { spHassIntegration->hassLoop(); }
}

float Controller::getDischargeCurrentLimit()
{
    auto const& config = Configuration.get();

    if (!config.Battery.EnableDischargeCurrentLimit) { return FLT_MAX; }

    /**
     * we are looking at two limits: (1) the static discharge current limit
     * setup by the user as part of the configuration, which is effective below
     * a (SoC or voltage) threshold, and (2) the dynamic discharge current
     * limit reported by the BMS.
     *
     * for both types of limits, we will determine its value, then test a bunch
     * of excuses why the limit might not be applicable.
     *
     * the smaller limit will be enforced, i.e., returned here.
     */
    auto spStats = getStats();

    auto getConfiguredLimit = [&config,&spStats]() -> float {
        auto configuredLimit = config.Battery.DischargeCurrentLimit;
        if (configuredLimit <= 0.0f) { return FLT_MAX; } // invalid setting

        bool useSoC = spStats->getSoCAgeSeconds() <= 60 && !config.PowerLimiter.IgnoreSoc;

        if (useSoC) {
            auto threshold = config.Battery.DischargeCurrentLimitBelowSoc;
            if (spStats->getSoC() >= threshold) { return FLT_MAX; }

            return configuredLimit;
        }

        bool voltageValid = spStats->getVoltageAgeSeconds() <= 60;
        if (voltageValid) {
            auto threshold = config.Battery.DischargeCurrentLimitBelowVoltage;
            if (spStats->getVoltage() >= threshold) { return FLT_MAX; }
        }

        return configuredLimit;
    };

    auto getBatteryLimit = [&config,&spStats]() -> float {
        if (!config.Battery.UseBatteryReportedDischargeCurrentLimit) { return FLT_MAX; }

        if (spStats->getDischargeCurrentLimitAgeSeconds() > 60) { return FLT_MAX; } // unusable

        return spStats->getDischargeCurrentLimit();
    };

    return std::min(getConfiguredLimit(), getBatteryLimit());
}

} // namespace Batteries

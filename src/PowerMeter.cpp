// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeter.h"
#include "Configuration.h"
#include "PowerMeterHttpJson.h"
#include "PowerMeterHttpSml.h"
#include "PowerMeterMqtt.h"
#include "PowerMeterSerialSdm.h"
#include "PowerMeterSerialSml.h"
#include "PowerMeterUdpSmaHomeManager.h"

PowerMeterClass PowerMeter;

void PowerMeterClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&PowerMeterClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    updateSettings();
}

void PowerMeterClass::updateSettings()
{
    std::lock_guard<std::mutex> l(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    auto const& config = Configuration.get();

    if (!config.PowerMeter.Enabled) { return; }

    switch(static_cast<PowerMeterProvider::Type>(config.PowerMeter.Source)) {
        case PowerMeterProvider::Type::MQTT:
            _upProvider = std::make_unique<PowerMeterMqtt>();
            break;
        case PowerMeterProvider::Type::SDM1PH:
        case PowerMeterProvider::Type::SDM3PH:
            _upProvider = std::make_unique<PowerMeterSerialSdm>();
            break;
        case PowerMeterProvider::Type::HTTP:
            _upProvider = std::make_unique<PowerMeterHttpJson>();
            break;
        case PowerMeterProvider::Type::SML:
            _upProvider = std::make_unique<PowerMeterSerialSml>();
            break;
        case PowerMeterProvider::Type::SMAHM2:
            _upProvider = std::make_unique<PowerMeterUdpSmaHomeManager>();
            break;
        case PowerMeterProvider::Type::TIBBER:
            _upProvider = std::make_unique<PowerMeterHttpSml>();
            break;
    }

    if (!_upProvider->init()) {
        _upProvider = nullptr;
    }
}

float PowerMeterClass::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return 0.0; }
    return _upProvider->getPowerTotal();
}

uint32_t PowerMeterClass::getLastUpdate() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return 0; }
    return _upProvider->getLastUpdate();
}

bool PowerMeterClass::isDataValid() const
{
    std::lock_guard<std::mutex> l(_mutex);
    if (!_upProvider) { return false; }
    return _upProvider->isDataValid();
}

void PowerMeterClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_upProvider) { return; }
    _upProvider->loop();
    _upProvider->mqttLoop();
}

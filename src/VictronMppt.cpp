// SPDX-License-Identifier: GPL-2.0-or-later
#include "VictronMppt.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "MessageOutput.h"

VictronMpptClass VictronMppt;

void VictronMpptClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&VictronMpptClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void VictronMpptClass::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _controllers.clear();

    CONFIG_T& config = Configuration.get();
    if (!config.Vedirect.Enabled) { return; }

    const PinMapping_t& pin = PinMapping.get();
    int8_t rx = pin.victron_rx;
    int8_t tx = pin.victron_tx;

    MessageOutput.printf("[VictronMppt] rx = %d, tx = %d\r\n", rx, tx);

    if (rx < 0) {
        MessageOutput.println(F("[VictronMppt] invalid pin config"));
        return;
    }

    auto upController = std::make_unique<VeDirectMpptController>();
    upController->init(rx, tx, &MessageOutput, config.Vedirect.VerboseLogging);
    _controllers.push_back(std::move(upController));
}

void VictronMpptClass::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto const& upController : _controllers) {
        upController->loop();
    }
}

bool VictronMpptClass::isDataValid() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto const& upController : _controllers) {
        if (!upController->isDataValid()) { return false; }
    }

    return !_controllers.empty();
}

uint32_t VictronMpptClass::getDataAgeMillis() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_controllers.empty()) { return 0; }

    auto now = millis();

    auto iter = _controllers.cbegin();
    uint32_t age = now - (*iter)->getLastUpdate();
    ++iter;

    while (iter != _controllers.end()) {
        age = std::min<uint32_t>(age, now - (*iter)->getLastUpdate());
        ++iter;
    }

    return age;
}

VeDirectMpptController::spData_t VictronMpptClass::getData(size_t idx) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_controllers.empty() || idx >= _controllers.size()) {
        MessageOutput.printf("ERROR: MPPT controller index %d is out of bounds (%d controllers)\r\n",
                idx, _controllers.size());
        return std::make_shared<VeDirectMpptController::veMpptStruct>();
    }

    return _controllers[idx]->getData();
}

int32_t VictronMpptClass::getPowerOutputWatts() const
{
    int32_t sum = 0;

    for (const auto& upController : _controllers) {
        sum += upController->getData()->P;
    }

    return sum;
}

int32_t VictronMpptClass::getPanelPowerWatts() const
{
    int32_t sum = 0;

    for (const auto& upController : _controllers) {
        sum += upController->getData()->PPV;
    }

    return sum;
}

double VictronMpptClass::getYieldTotal() const
{
    double sum = 0;

    for (const auto& upController : _controllers) {
        sum += upController->getData()->H19;
    }

    return sum;
}

double VictronMpptClass::getYieldDay() const
{
    double sum = 0;

    for (const auto& upController : _controllers) {
        sum += upController->getData()->H20;
    }

    return sum;
}

// SPDX-License-Identifier: GPL-2.0-or-later
#include "VictronMppt.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "MessageOutput.h"
#include "SerialPortManager.h"

VictronMpptClass VictronMppt;

void VictronMpptClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback([this] { loop(); });
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void VictronMpptClass::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _controllers.clear();
    PortManager.invalidateMpptPorts();

    CONFIG_T& config = Configuration.get();
    if (!config.Vedirect.Enabled) { return; }

    const PinMapping_t& pin = PinMapping.get();

    int hwSerialPort = 1;
    bool initSuccess = initController(pin.victron_rx, pin.victron_tx, config.Vedirect.VerboseLogging, hwSerialPort);
    if (initSuccess) {
        hwSerialPort++;
    }

    initController(pin.victron_rx2, pin.victron_tx2, config.Vedirect.VerboseLogging, hwSerialPort);
}

bool VictronMpptClass::initController(int8_t rx, int8_t tx, bool logging, int hwSerialPort)
{
    MessageOutput.printf("[VictronMppt] rx = %d, tx = %d, hwSerialPort = %d\r\n", rx, tx, hwSerialPort);

    if (rx < 0) {
        MessageOutput.printf("[VictronMppt] invalid pin config rx = %d, tx = %d\r\n", rx, tx);
        return false;
    }

    if (!PortManager.allocateMpptPort(hwSerialPort)) {
        MessageOutput.printf("[VictronMppt] Serial port %d already in use. Initialization aborted!\r\n",
                             hwSerialPort);
        return false;
    }

    auto upController = std::make_unique<VeDirectMpptController>();
    upController->init(rx, tx, &MessageOutput, logging, hwSerialPort);
    _controllers.push_back(std::move(upController));
    return true;
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

    for (auto const& upController: _controllers) {
        if (!upController->isDataValid()) { return false; }
    }

    return !_controllers.empty();
}

bool VictronMpptClass::isDataValid(size_t idx) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_controllers.empty() || idx >= _controllers.size()) {
        return false;
    }

    return _controllers[idx]->isDataValid();
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

std::optional<VeDirectMpptController::spData_t> VictronMpptClass::getData(size_t idx) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_controllers.empty() || idx >= _controllers.size()) {
        MessageOutput.printf("ERROR: MPPT controller index %d is out of bounds (%d controllers)\r\n",
                             idx, _controllers.size());
        return std::nullopt;
    }

    return std::optional<VeDirectMpptController::spData_t>{_controllers[idx]->getData()};
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

double VictronMpptClass::getOutputVoltage() const
{
    double min = -1;

    for (const auto& upController : _controllers) {
        double volts = upController->getData()->V;
        if (min == -1) { min = volts; }
        min = std::min(min, volts);
    }

    return min;
}

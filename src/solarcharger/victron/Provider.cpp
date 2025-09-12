// SPDX-License-Identifier: GPL-2.0-or-later
#include <solarcharger/victron/Provider.h>
#include "Configuration.h"
#include "PinMapping.h"
#include "SerialPortManager.h"
#include <LogHelper.h>

#undef TAG
static const char* TAG = "solarCharger";
static const char* SUBTAG = "VE.Direct";

namespace SolarChargers::Victron {

bool Provider::init()
{
    const PinMapping_t& pin = PinMapping.get();
    auto controllerCount = 0;

    if (initController(pin.victron_rx, pin.victron_tx, 1)) {
        controllerCount++;
    }

    if (initController(pin.victron_rx2, pin.victron_tx2, 2)) {
        controllerCount++;
    }

    if (initController(pin.victron_rx3, pin.victron_tx3, 3)) {
        controllerCount++;
    }

    return controllerCount > 0;
}

void Provider::deinit()
{
    std::lock_guard<std::mutex> lock(_mutex);

    _controllers.clear();
    for (auto const& o: _serialPortOwners) {
        SerialPortManager.freePort(o.c_str());
    }
    _serialPortOwners.clear();
}

bool Provider::initController(gpio_num_t rx, gpio_num_t tx, uint8_t instance)
{
    DTU_LOGI("Instance %d: rx = %d, tx = %d", instance, rx, tx);

    if (rx <= GPIO_NUM_NC) {
        DTU_LOGE("Instance %d: invalid pin config", instance);
        return false;
    }

    String owner("Victron MPPT ");
    owner += String(instance);
    auto oHwSerialPort = SerialPortManager.allocatePort(owner.c_str());
    if (!oHwSerialPort) { return false; }

    _serialPortOwners.push_back(owner);

    auto upController = std::make_unique<VeDirectMpptController>();
    upController->init(rx, tx, *oHwSerialPort);
    _controllers.push_back(std::move(upController));
    return true;
}

void Provider::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    for (auto const& upController : _controllers) {
        upController->loop();

        if (upController->isDataValid()) {
            _stats->update(upController->getLogId(), upController->getData(), upController->getLastUpdate());
        }
    }
}

} // namespace SolarChargers::Victron

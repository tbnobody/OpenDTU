// SPDX-License-Identifier: GPL-2.0-or-later
#include "VictronSmartShunt.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "MessageOutput.h"


bool VictronSmartShunt::init(bool verboseLogging)
{
    MessageOutput.println("[VictronSmartShunt] Initialize interface...");

    const PinMapping_t& pin = PinMapping.get();
    MessageOutput.printf("[VictronSmartShunt] Interface rx = %d, tx = %d\r\n",
            pin.battery_rx, pin.battery_tx);

    if (pin.battery_rx < 0) {
        MessageOutput.println("[VictronSmartShunt] Invalid pin config");
        return false;
    }

    auto tx = static_cast<gpio_num_t>(pin.battery_tx);
    auto rx = static_cast<gpio_num_t>(pin.battery_rx);

    VeDirectShunt.init(rx, tx, &MessageOutput, verboseLogging);
    return true;
}

void VictronSmartShunt::loop()
{
    VeDirectShunt.loop();

    if (VeDirectShunt.getLastUpdate() <= _lastUpdate) { return; }

    _stats->updateFrom(VeDirectShunt.veFrame);
    _lastUpdate = VeDirectShunt.getLastUpdate();
}

bool VictronSmartShunt::usesHwPort2() {
    return true;
}

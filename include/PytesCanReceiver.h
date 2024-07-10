// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Configuration.h"
#include "Battery.h"
#include "BatteryCanReceiver.h"
#include <driver/twai.h>

class PytesCanReceiver : public BatteryCanReceiver {
public:
    bool init(bool verboseLogging) final;
    void onMessage(twai_message_t rx_message) final;

    std::shared_ptr<BatteryStats> getStats() const final { return _stats; }

private:
    std::shared_ptr<PytesBatteryStats> _stats =
        std::make_shared<PytesBatteryStats>();
};

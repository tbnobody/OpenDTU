// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Battery.h"

class VictronSmartShunt : public BatteryProvider {
public:
    bool init(bool verboseLogging) final;
    void deinit() final { }
    void loop() final;
    std::shared_ptr<BatteryStats> getStats() const final { return _stats; }
    bool usesHwPort2() const final {
        return ARDUINO_USB_CDC_ON_BOOT != 1;
    }

private:
    uint32_t _lastUpdate = 0;
    std::shared_ptr<VictronSmartShuntStats> _stats =
        std::make_shared<VictronSmartShuntStats>();
};

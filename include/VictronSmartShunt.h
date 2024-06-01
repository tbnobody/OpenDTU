// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "Battery.h"

class VictronSmartShunt : public BatteryProvider {
public:
    bool init(bool verboseLogging) final;
    void deinit() final { }
    void loop() final;
    std::shared_ptr<BatteryStats> getStats() const final { return _stats; }
    int usedHwUart() const final { return _hwSerialPort; }

private:
    static uint8_t constexpr _hwSerialPort = ((ARDUINO_USB_CDC_ON_BOOT != 1)?2:0);
    uint32_t _lastUpdate = 0;
    std::shared_ptr<VictronSmartShuntStats> _stats =
        std::make_shared<VictronSmartShuntStats>();
};

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerLimiterInverter.h"

class PowerLimiterSolarInverter : public PowerLimiterInverter {
public:
    PowerLimiterSolarInverter(bool verboseLogging, PowerLimiterInverterConfig const& config);

    uint16_t getMaxReductionWatts(bool allowStandby) const final;
    uint16_t getMaxIncreaseWatts() const final;
    uint16_t applyReduction(uint16_t reduction, bool allowStandby) final;
    uint16_t applyIncrease(uint16_t increase) final;
    uint16_t standby() final;
    bool isSolarPowered() const final { return true; }

private:
    uint16_t scaleLimit(uint16_t expectedOutputWatts);
    void setAcOutput(uint16_t expectedOutputWatts) final;
};

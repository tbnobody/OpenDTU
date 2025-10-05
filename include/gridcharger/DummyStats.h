// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gridcharger/Stats.h>

namespace GridChargers {

class DummyStats : public Stats {
public:
    uint32_t getLastUpdate() const final { return 0; }
    std::optional<float> getInputPower() const final { return std::nullopt; }
    void getLiveViewData(JsonVariant& root) const final {;}
    void mqttPublish() const final {;}
};

} // namespace GridChargers

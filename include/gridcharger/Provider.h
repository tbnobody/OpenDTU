// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <gridcharger/Stats.h>

namespace GridChargers {

class Provider {
public:
    virtual ~Provider() = default;
    virtual bool init() = 0;
    virtual void deinit() = 0;
    virtual void loop() = 0;

    virtual std::shared_ptr<Stats> getStats() const = 0;
    virtual bool getAutoPowerStatus() const = 0;
};

} // namespace GridChargers

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerMeterProvider.h"
#include <TaskSchedulerDeclarations.h>
#include <memory>
#include <mutex>

class PowerMeterClass {
public:
    void init(Scheduler& scheduler);

    void updateSettings();

    float getPowerTotal() const;
    uint32_t getLastUpdate() const;
    bool isDataValid() const;

private:
    void loop();

    Task _loopTask;
    mutable std::mutex _mutex;
    std::unique_ptr<PowerMeterProvider> _upProvider = nullptr;
};

extern PowerMeterClass PowerMeter;

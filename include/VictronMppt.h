// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <mutex>
#include <memory>

#include "VeDirectMpptController.h"
#include <TaskSchedulerDeclarations.h>

class VictronMpptClass {
public:
    VictronMpptClass() = default;
    ~VictronMpptClass() = default;

    void init(Scheduler& scheduler);
    void updateSettings();

    bool isDataValid() const;

    // returns the data age of all controllers,
    // i.e, the youngest data's age is returned.
    uint32_t getDataAgeMillis() const;

    VeDirectMpptController::spData_t getData(size_t idx = 0) const;

    // total output of all MPPT charge controllers in Watts
    int32_t getPowerOutputWatts() const;

    // total panel input power of all MPPT charge controllers in Watts
    int32_t getPanelPowerWatts() const;

    // sum of total yield of all MPPT charge controllers in kWh
    double getYieldTotal() const;

    // sum of today's yield of all MPPT charge controllers in kWh
    double getYieldDay() const;

private:
    void loop();
    VictronMpptClass(VictronMpptClass const& other) = delete;
    VictronMpptClass(VictronMpptClass&& other) = delete;
    VictronMpptClass& operator=(VictronMpptClass const& other) = delete;
    VictronMpptClass& operator=(VictronMpptClass&& other) = delete;

    Task _loopTask;

    mutable std::mutex _mutex;
    using controller_t = std::unique_ptr<VeDirectMpptController>;
    std::vector<controller_t> _controllers;
};

extern VictronMpptClass VictronMppt;

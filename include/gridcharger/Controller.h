// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <mutex>
#include <TaskSchedulerDeclarations.h>
#include <gridcharger/Provider.h>

namespace GridChargers {

class Controller {
public:
    void init(Scheduler&);
    void updateSettings();

    std::shared_ptr<Stats const> getStats() const;
    bool getAutoPowerStatus() const;

    // Simple template method for provider-specific access
    template<typename T>
    T* getProvider() const;

private:
    void loop() const;

    Task _loopTask;
    mutable std::mutex _mutex;
    std::unique_ptr<Provider> _upProvider = nullptr;
};

} // namespace GridChargers

extern GridChargers::Controller GridCharger;

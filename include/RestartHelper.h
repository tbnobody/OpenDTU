// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <atomic>

class RestartHelperClass {
public:
    RestartHelperClass();
    void init(Scheduler& scheduler);
    // saveRuntimeData = false skips writing the runtime data file before the restart,
    // e.g. after a factory reset removed all files
    void triggerRestart(bool saveRuntimeData = true);

private:
    void loop();

    Task _rebootTask;
    std::atomic<bool> _saveRuntimeData = true; // set from the web server task, read in the main loop
};

extern RestartHelperClass RestartHelper;

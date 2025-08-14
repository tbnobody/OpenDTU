// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <WString.h>
#include <vector>

class LoggingClass {
public:
    LoggingClass();
    void applyLogLevels();
    const std::vector<String>& getConfigurableModules() const;

private:
    std::vector<String> _configurableModules;
};

extern LoggingClass Logging;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <mutex>

class CpuTemperatureClass {
public:
    float read();

private:
    std::mutex _mutex;
};

extern CpuTemperatureClass CpuTemperature;

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include "Configuration.h"

class FailsafeCheckClass {
public:
    void init();
    void loop();
    void RequestReceived(uint8_t pos);

private:
    uint64_t _lastUpdate[INV_MAX_COUNT];
    uint64_t _lastCurrentCheck;
    
};

extern FailsafeCheckClass FailsafeCheck;
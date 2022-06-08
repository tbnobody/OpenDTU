#pragma once

#include <stdint.h>

class TimeoutHelper {
public:
    TimeoutHelper();
    void set(uint32_t ms);
    void extend(uint32_t ms);
    bool occured();

private:
    uint32_t startMillis;
    uint32_t timeout;
};
#pragma once

#include <time.h>

class Clock {

public:
    virtual int getTimezoneOffset() = 0;

    /**
     * @brief Gets the instant in epoch time. Returns false if no time is available
     */
    virtual bool getNow(time_t* timePtr) = 0;
};
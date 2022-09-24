#include "Clock.h"

class Clock_Impl : public Clock {
public:
    int getTimezoneOffset();
    bool getNow(time_t* nowPtr);
};
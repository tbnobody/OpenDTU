#pragma once

#include "CommandAbstract.h"
#include <sys/time.h>

class MultiDataCommand : public CommandAbstract {
public:
    MultiDataCommand(uint64_t target_address = 0, uint64_t router_address = 0, uint8_t data_type = 0, time_t time = 0);

    void setTime(time_t time);
    time_t getTime();

protected:
    void setDataType(uint8_t data_type);
    uint8_t getDataType();
};
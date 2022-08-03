#pragma once

#include "CommandAbstract.h"

class DevControlCommand : public CommandAbstract {
public:
    DevControlCommand(uint64_t target_address = 0, uint64_t router_address = 0);
};
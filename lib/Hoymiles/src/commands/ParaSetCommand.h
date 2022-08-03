#pragma once

#include "CommandAbstract.h"

class ParaSetCommand : public CommandAbstract {
public:
    ParaSetCommand(uint64_t target_address = 0, uint64_t router_address = 0);
};
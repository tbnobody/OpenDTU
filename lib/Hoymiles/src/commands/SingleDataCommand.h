#pragma once

#include "CommandAbstract.h"

class SingleDataCommand : public CommandAbstract {
public:
    SingleDataCommand(uint64_t target_address = 0, uint64_t router_address = 0);

    RequestType getRequestType() { return RequestType::None; };
};
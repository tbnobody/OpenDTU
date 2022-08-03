#include "ParaSetCommand.h"

ParaSetCommand::ParaSetCommand(uint64_t target_address, uint64_t router_address)
    : CommandAbstract(target_address, router_address)
{
    _payload[0] = 0x52;
}
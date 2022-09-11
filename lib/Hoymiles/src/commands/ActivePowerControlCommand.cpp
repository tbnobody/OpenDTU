#include "ActivePowerControlCommand.h"

ActivePowerControlCommand::ActivePowerControlCommand(uint64_t target_address, uint64_t router_address)
    : DevControlCommand(target_address, router_address)
{
}

bool ActivePowerControlCommand::handleResponse(InverterAbstract* inverter, fragment_t fragment[], uint8_t max_fragment_id)
{
    return true;
}
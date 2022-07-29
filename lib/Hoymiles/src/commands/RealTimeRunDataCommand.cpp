#include "RealTimeRunDataCommand.h"

RealTimeRunDataCommand::RealTimeRunDataCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x0b);
    setTimeout(200);
}
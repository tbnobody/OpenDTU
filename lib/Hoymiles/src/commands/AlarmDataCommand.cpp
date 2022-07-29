#include "AlarmDataCommand.h"

AlarmDataCommand::AlarmDataCommand(uint64_t target_address, uint64_t router_address, time_t time)
    : MultiDataCommand(target_address, router_address)
{
    setTime(time);
    setDataType(0x11);
    setTimeout(200);
}
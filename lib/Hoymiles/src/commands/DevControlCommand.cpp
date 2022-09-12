#include "DevControlCommand.h"
#include "crc.h"

DevControlCommand::DevControlCommand(uint64_t target_address, uint64_t router_address)
    : CommandAbstract(target_address, router_address)
{
    _payload[0] = 0x51;
    _payload[9] = 0x81;
}

void DevControlCommand::udpateCRC(uint8_t len)
{
    uint16_t crc = crc16(&_payload[10], len);
    _payload[10 + len] = (uint8_t)(crc >> 8);
    _payload[10 + len + 1] = (uint8_t)(crc);
}
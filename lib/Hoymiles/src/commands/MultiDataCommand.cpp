#include "MultiDataCommand.h"
#include "crc.h"

MultiDataCommand::MultiDataCommand(uint64_t target_address, uint64_t router_address, uint8_t data_type, time_t time)
    : CommandAbstract(target_address, router_address)
{
    _payload[0] = 0x15;
    _payload[9] = 0x80;
    setDataType(data_type);
    _payload[11] = 0x00;
    setTime(time);
    _payload[16] = 0x00; // Gap
    _payload[17] = 0x00; // Gap
    _payload[18] = 0x00;
    _payload[19] = 0x00;
    _payload[20] = 0x00; // Password
    _payload[21] = 0x00; // Password
    _payload[22] = 0x00; // Password
    _payload[23] = 0x00; // Password

    uint16_t crc = crc16(&_payload[10], 14); // From data_type till password
    _payload[24] = (uint8_t)(crc >> 8);
    _payload[25] = (uint8_t)(crc);

    _payload_size = 26;
}

void MultiDataCommand::setDataType(uint8_t data_type)
{
    _payload[10] = data_type;
}
uint8_t MultiDataCommand::getDataType()
{
    return _payload[10];
}

void MultiDataCommand::setTime(time_t time)
{
    _payload[12] = (uint8_t)(time >> 24);
    _payload[13] = (uint8_t)(time >> 16);
    _payload[14] = (uint8_t)(time >> 8);
    _payload[15] = (uint8_t)(time);
}

time_t MultiDataCommand::getTime()
{
    return (time_t)(_payload[12] << 24)
        | (time_t)(_payload[13] << 16)
        | (time_t)(_payload[14] << 8)
        | (time_t)(_payload[15]);
}
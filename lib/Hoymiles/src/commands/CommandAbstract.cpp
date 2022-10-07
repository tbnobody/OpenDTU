#include "CommandAbstract.h"
#include "crc.h"
#include <string.h>

CommandAbstract::CommandAbstract(uint64_t target_address, uint64_t router_address)
{
    memset(_payload, 0, RF_LEN);
    _payload_size = 0;

    setTargetAddress(target_address);
    setRouterAddress(router_address);
    setSendCount(0);
    setTimeout(0);
}

const uint8_t* CommandAbstract::getDataPayload()
{
    _payload[_payload_size] = crc8(_payload, _payload_size);
    return _payload;
}

void CommandAbstract::dumpDataPayload(Stream& stream)
{
    const uint8_t* payload = getDataPayload();
    for (uint8_t i = 0; i < getDataSize(); i++) {
        stream.printf("%02X ", payload[i]);
    }
    stream.println("");
}

uint8_t CommandAbstract::getDataSize()
{
    return _payload_size + 1; // Original payload plus crc8
}

void CommandAbstract::setTargetAddress(uint64_t address)
{
    convertSerialToPacketId(&_payload[1], address);
    _targetAddress = address;
}
const uint64_t CommandAbstract::getTargetAddress()
{
    return _targetAddress;
}

void CommandAbstract::setRouterAddress(uint64_t address)
{
    convertSerialToPacketId(&_payload[5], address);
    _routerAddress = address;
}

const uint64_t CommandAbstract::getRouterAddress()
{
    return _routerAddress;
}

void CommandAbstract::setTimeout(uint32_t timeout)
{
    _timeout = timeout;
}

uint32_t CommandAbstract::getTimeout()
{
    return _timeout;
}

void CommandAbstract::setSendCount(uint8_t count)
{
    _sendCount = count;
}

uint8_t CommandAbstract::getSendCount()
{
    return _sendCount;
}

uint8_t CommandAbstract::incrementSendCount()
{
    return _sendCount++;
}

CommandAbstract* CommandAbstract::getRequestFrameCommand(uint8_t frame_no)
{
    return nullptr;
}

void CommandAbstract::convertSerialToPacketId(uint8_t buffer[], uint64_t serial)
{
    serial_u s;
    s.u64 = serial;
    buffer[3] = s.b[0];
    buffer[2] = s.b[1];
    buffer[1] = s.b[2];
    buffer[0] = s.b[3];
}

void CommandAbstract::gotTimeout(InverterAbstract* inverter)
{
}
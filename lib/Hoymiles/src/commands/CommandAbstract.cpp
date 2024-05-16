// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

/*
Command structure:
* Each package has a maximum of 32 bytes
* Target Address: the address of the inverter. Has to be read as hex value
* Source Address the address of the dtu itself. Has to be read as hex value
* CRC8: a crc8 checksum added to the end of the payload containing all valid data.
  Each sub-commmand has to set it's own payload size.

Conversion of Target Addr:
Inverter Serial Number: (0x)116171603546
Target Address: 71 60 35 46

Conversion of Source Addr:
DTU Serial Number: (0x)199980122304
Source Address: 80 12 23 04

00   01 02 03 04   05 06 07 08   09 10   11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-----------------------------------------------------------------------------------------------------
|<------------- CRC8 ------------>|
00   71 60 35 46   80 12 23 04   00 00   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
     ^^^^^^^^^^^   ^^^^^^^^^^^      ^^
     Target Addr   Source Addr      CRC8
*/
#include "CommandAbstract.h"
#include "crc.h"
#include <string.h>
#include "../inverters/InverterAbstract.h"

CommandAbstract::CommandAbstract(InverterAbstract* inv, const uint64_t router_address)
{
    memset(_payload, 0, RF_LEN);
    _payload_size = 0;

    _inv = inv;

    setTargetAddress(_inv->serial());
    setRouterAddress(router_address);
    setSendCount(0);
    setTimeout(0);
}

const uint8_t* CommandAbstract::getDataPayload()
{
    _payload[_payload_size] = crc8(_payload, _payload_size);
    return _payload;
}

void CommandAbstract::dumpDataPayload(Print* stream)
{
    const uint8_t* payload = getDataPayload();
    for (uint8_t i = 0; i < getDataSize(); i++) {
        stream->printf("%02X ", payload[i]);
    }
    stream->println("");
}

uint8_t CommandAbstract::getDataSize() const
{
    return _payload_size + 1; // Original payload plus crc8
}

void CommandAbstract::setTargetAddress(const uint64_t address)
{
    convertSerialToPacketId(&_payload[1], address);
    _targetAddress = address;
}
uint64_t CommandAbstract::getTargetAddress() const
{
    return _targetAddress;
}

void CommandAbstract::setRouterAddress(const uint64_t address)
{
    convertSerialToPacketId(&_payload[5], address);
    _routerAddress = address;
}

uint64_t CommandAbstract::getRouterAddress() const
{
    return _routerAddress;
}

void CommandAbstract::setTimeout(const uint32_t timeout)
{
    _timeout = timeout;
}

uint32_t CommandAbstract::getTimeout() const
{
    return _timeout;
}

void CommandAbstract::setSendCount(const uint8_t count)
{
    _sendCount = count;
}

uint8_t CommandAbstract::getSendCount() const
{
    return _sendCount;
}

uint8_t CommandAbstract::incrementSendCount()
{
    return _sendCount++;
}

CommandAbstract* CommandAbstract::getRequestFrameCommand(const uint8_t frame_no)
{
    return nullptr;
}

void CommandAbstract::convertSerialToPacketId(uint8_t buffer[], const uint64_t serial)
{
    serial_u s;
    s.u64 = serial;
    buffer[3] = s.b[0];
    buffer[2] = s.b[1];
    buffer[1] = s.b[2];
    buffer[0] = s.b[3];
}

void CommandAbstract::gotTimeout()
{
}

uint8_t CommandAbstract::getMaxResendCount() const
{
    return MAX_RESEND_COUNT;
}

uint8_t CommandAbstract::getMaxRetransmitCount() const
{
    return MAX_RETRANSMIT_COUNT;
}

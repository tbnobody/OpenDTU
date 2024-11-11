
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

/*
Derives from CommandAbstract. Has a variable length.

Command structure:
* ID: fixed identifier and everytime 0x51
* Cmd: Fixed at 0x81 for these types of commands
* Payload: dynamic amount of bytes
* CRC16: calcuclated over the highlighted amount of bytes

00   01 02 03 04   05 06 07 08   09   10 11   12   13   14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
-------------------------------------------------------------------------------------------------------------
                                      |<->| CRC16
51   71 60 35 46   80 12 23 04   81   00 00   00   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^^^^   ^^   ^^
ID   Target Addr   Source Addr   Cmd  Payload CRC16 CRC8
*/
#include "DevControlCommand.h"
#include "crc.h"

DevControlCommand::DevControlCommand(InverterAbstract* inv, const uint64_t router_address)
    : CommandAbstract(inv, router_address)
{
    _payload[0] = 0x51;
    _payload[9] = 0x81;

    setTimeout(1000);
}

void DevControlCommand::udpateCRC(const uint8_t len)
{
    const uint16_t crc = crc16(&_payload[10], len);
    _payload[10 + len] = static_cast<uint8_t>(crc >> 8);
    _payload[10 + len + 1] = static_cast<uint8_t>(crc);
}

bool DevControlCommand::handleResponse(const fragment_t fragment[], const uint8_t max_fragment_id)
{
    for (uint8_t i = 0; i < max_fragment_id; i++) {
        if (fragment[i].mainCmd != (_payload[0] | 0x80)) {
            return false;
        }
    }

    return true;
}

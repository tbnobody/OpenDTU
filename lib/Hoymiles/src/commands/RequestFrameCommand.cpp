// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2023 Thomas Basler and others
 */

/*
This command is used to re-request a specific fragment returned by a MultiDataCommand from the inverter.

Derives from SingleDataCommand. Has a fixed length of 10 bytes.

Command structure:
* ID: fixed identifier and everytime 0x15
* Idx: the counter of sequencial packages to send. Currently it's only 0x80
  because all request requests only consist of one package.
* Frm: is set to the fragment id to re-request. "Or" operation with 0x80 is applied to the frame.

00   01 02 03 04   05 06 07 08   09   10   11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
---------------------------------------------------------------------------------------------------------
15   71 60 35 46   80 12 23 04   85   00   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
^^   ^^^^^^^^^^^   ^^^^^^^^^^^   ^^   ^^
ID   Target Addr   Source Addr   Frm  CRC8
*/
#include "RequestFrameCommand.h"

RequestFrameCommand::RequestFrameCommand(const uint64_t target_address, const uint64_t router_address, uint8_t frame_no)
    : SingleDataCommand(target_address, router_address)
{
    if (frame_no > 127) {
        frame_no = 0;
    }
    setFrameNo(frame_no);
    _payload_size = 10;
}

String RequestFrameCommand::getCommandName() const
{
    return "RequestFrame";
}

void RequestFrameCommand::setFrameNo(const uint8_t frame_no)
{
    _payload[9] = frame_no | 0x80;
}

uint8_t RequestFrameCommand::getFrameNo() const
{
    return _payload[9] & (~0x80);
}

bool RequestFrameCommand::handleResponse(InverterAbstract& inverter, const fragment_t fragment[], const uint8_t max_fragment_id)
{
    return true;
}